/**
 * This is the core implementation of mx-server daemon service that uses gRPC and supports multiprocessing and over the 
 * network communication with the MPU.
 * 
 */

#include "memx/accl/DeviceManager.h"
#include <memx/memx.h>
#include <csignal>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"
#include "absl/log/check.h"

#include "mx_proc.grpc.pb.h"
#include "mx_proc.pb.h"

//Hyper parameter for the number if completion queus performing the ofmap task
#define NUM_OFMAP_CQS 1
//The maximum number of groups supported. Set to 16 because currently only 32 contexts are supported 
//by the udriver and we allocate two contexts for each group
#define NUM_MAX_GROUPS 16

using mxstream::MxService;
using mxstream::LockData;
using mxstream::DfpData;
using mxstream::Ping;
using mxstream::MxData;
using mxstream::Uuid;

atomic_bool server_shutdown_flag;

std::vector<MX::Utils::fifo_deque<std::string,int>> uuid_q(NUM_MAX_GROUPS);
std::vector<std::mutex> send_mutex(NUM_MAX_GROUPS);
std::vector<std::mutex> queue_mutex(NUM_MAX_GROUPS);
std::vector<std::condition_variable> queue_cv(NUM_MAX_GROUPS);

std::unique_ptr<grpc::Server> sync_server;

/**
Shutdown the gRPC server when a signal is received.
This is a simple function that stores a shutdown flag and calls
Shutdown() on the server.
*/
void signalHandler(int signum) {
    std::cout << "Signal " << signum << " shutting down the daemon" << std::endl;
    server_shutdown_flag.store(true);
    if(sync_server){
        sync_server->Shutdown();
    }
}

/**
 * Class that implemets are the initialization and exit functions of mx-accl.
 * This is implemented using Syncronous RPCs as these functions are not continuously called
 * and simplicity is preferred over performance.
 */
class MxServiceImpl final: public MxService::Service{
    public:
        MxServiceImpl(){
            //Heartbeat thread that checks if a client is alive by continously receiving a heartbeat message
            heartbeat_thread = new std::thread(&MxServiceImpl::heartbeat_check,this);
            int all_devices_count;
            memx_status status = memx_operation_get_device_count(&all_devices_count);
            if memx_status_error(status){
                printf("daemon failed to get device count\n");
            }

            //MPU lock handled by daemon
            device_locks = new std::vector<atomic_bool>(all_devices_count);

            //Required for local heartbeat check
            local_threads.resize(all_devices_count,NULL);
            local_run_flags.resize(all_devices_count,false);
            local_times.resize(all_devices_count,std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()));
        }

        ~MxServiceImpl(){
            if(device_manager){
                device_manager->cleanup__all_dfps();
                device_manager->close_all_devices();
                delete device_manager;
            }
            if(device_locks != NULL){
                delete device_locks;
            }
            heartbeat_thread->join();
            delete heartbeat_thread;
        }

        grpc::Status try_lock(grpc::ServerContext*, const LockData* grp_id, Ping* reply) override{
            if((*device_locks)[grp_id->group_id()] == false){
                (*device_locks)[grp_id->group_id()] = true;//Setting lock to true so that no other process can acquire the lock
                //Local heartbeat check to see if a local client is still alive
                local_threads[grp_id->group_id()] = new std::thread(&MxServiceImpl::local_heartbeat_check,this,grp_id->group_id());
                local_run_flags[grp_id->group_id()] = true;
                local_times[grp_id->group_id()] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
                reply->set_recv(true);
            }
            else{
                reply->set_recv(false);
            }
            return grpc::Status::OK;
        }

        /**
         * This RPC is used to connect a dfp to a group of devices. If the same dfp is already connected to a group, then the
         * process counter for that group is incremented. If the dfp is not connected then it is connected to the group. If a different
         * dfp tries to connect to the same group then the client is rejected.
         * The RPC is blocking until all the devices in the group are connected.
         * @param dfp_data The dfp data that includes the dfp bytes and the group ids to which the dfp needs to be connected
         * @param reply     The reply which includes a boolean indicating if the dfp was successfully connected and a string
         *                  message that can be used in case of failure
         */
        grpc::Status connect_dfp(grpc::ServerContext*, const DfpData* dfp_data, Ping* reply) override{
            reply->set_recv(true);
            std::vector<int> device_ids_to_use;
            std::string dfp_bytes_str =  dfp_data->dfp_bytes();
            const uint8_t* dfp_bytes = (uint8_t*)dfp_bytes_str.data();
            Dfp::DfpObject dfp_obj(dfp_bytes);// DfpObject to parse the dfp for meta data
            if(dfp_obj.get_dfp_meta().dfp_version <= 5){
                reply->set_recv(false);
                reply->set_msg("unsupported dfp version passed. Dfp version should be more than 6");
                return grpc::Status::OK;
            } 
            for(int i=0; i<dfp_data->group_id_size();++i){
                int group_id = dfp_data->group_id(i);
                if(device_dfp_map.find(group_id) != device_dfp_map.end()){//This group_id is currently busy serving an existing dfp
                    if( device_dfp_map.at(group_id) != dfp_obj.get_dfp_meta().hardware_hash){//The new dfp is different from the exisiting dfp
                        reply->set_recv(false);
                        reply->set_msg("A process with a different dfp is active on group: "+std::to_string(group_id)+". Must use same dfp in multiple processes or wait for the other processes to end first");
                        return grpc::Status::OK;
                    }
                    //Keep track of number of clients running on this group
                    device_process_counter[group_id]++;
                }
                else{//This group_id is free
                    device_ids_to_use.push_back(group_id);
                }
            }

            //Create a dummy of dfp_data to send to heatbeat thread
            DfpData* dummy_dfp = new DfpData;
            for(int i=0; i<dfp_data->group_id_size();++i){
                dummy_dfp->add_group_id(dfp_data->group_id(i));
            }
            dummy_dfp->set_uuid(dfp_data->uuid());
            dummy_dfp->set_dfp_bytes(dfp_data->dfp_bytes());
            uuid_dfp_map[dfp_data->uuid()] = dummy_dfp;

            if(device_ids_to_use.size()==0){
                return grpc::Status::OK;
            }

            if(device_manager==NULL){//Create a new device manager if it does not exist
                device_manager = new MX::Runtime::DeviceManager(NULL, true);
            }
            int dfp_tag = 0;
            //Find if this dfp has been sent before so that we can resuse the tag for metadata optimizing the memory usage
            if(checksum_tag_map.find(dfp_obj.get_dfp_meta().hardware_hash) == checksum_tag_map.end()){
                checksum_tag_map[dfp_obj.get_dfp_meta().hardware_hash] = dfp_tag_total;//Store the bag based on hash of hardware dfp
                dfp_tag = dfp_tag_total;
                dfp_tag_total+=1;
            }
            else{
                dfp_tag = checksum_tag_map[dfp_obj.get_dfp_meta().hardware_hash];
            }


            //Set all the dfp related metadata
            mx_retval_t open_ret = device_manager->opendfp_bytes(dfp_bytes, dfp_tag);
            reply->set_recv(open_ret.error_flag);
            if(!open_ret.error_flag){
                reply->set_msg(open_ret.error_msg);
                return grpc::Status::OK;
            }
            
            //Check if the given dfp is valid
            int dfp_valid = device_manager->get_dfp_validity(dfp_tag);
            if(!dfp_valid){
                reply->set_recv(false);
                reply->set_msg("Dfp is invalid, check the path and file");
                return grpc::Status::OK;
            }

            // exploit the fact that we only have 1 valid DFP per device at a time
            // FIXME: change in the future
            for( int gid : device_ids_to_use ){
                if((*device_locks)[gid]){
                    // this device is already locked!
                    reply->set_recv(false);
                    std::string err_msg = "Couldn't acquire lock on device ";
                    err_msg += std::to_string(gid);
                    reply->set_msg(err_msg.c_str());
                    return grpc::Status::OK;
                } else {
                    // else acquire this lock
                    (*device_locks)[gid] = true;
                }
            }

            //Setup the MXA belonging to the requested group
            mx_retval_t setup_ret =  device_manager->setup_mxa(dfp_tag, device_ids_to_use);
            if(!setup_ret.error_flag){
                reply->set_recv(false);
                reply->set_msg(setup_ret.error_msg);
                return grpc::Status::OK;
            }
            
            //Open the contexts in the udriver based on the type of dfp sent
            mx_retval_t attach_ret =  device_manager->attach_dfp_to_device(dfp_tag);
            if(!attach_ret.error_flag){
                reply->set_recv(false);
                reply->set_msg(attach_ret.error_msg);
                return grpc::Status::OK;
            }

            //Download the dfp to the MXA
            mx_retval_t download_ret =  device_manager->download_dfp_to_device(dfp_tag);
            if(!download_ret.error_flag){
                reply->set_recv(false);
                reply->set_msg(download_ret.error_msg);
                return grpc::Status::OK;
            }

            if(reply->recv()){
                //If everything is good then update the device_dfp_map and device_process_counter
                for(int i=0; i<static_cast<int>(device_ids_to_use.size());++i){
                    int group_id = device_ids_to_use[i];
                    device_dfp_map[group_id] = dfp_obj.get_dfp_meta().hardware_hash;
                    device_process_counter[group_id] = 1;
                }
            }
            return grpc::Status::OK;
        }

        /**
         * Closes the process associated with the provided DfpData, releasing any resources
         * allocated for the specified groups. Checks if the DFP metadata matches the connected
         * DFP for each group ID and decrements the process counter. If the counter reaches zero,
         * the device is closed and the lock is released. Deletes and erases UUID related mappings.
         * 
         */
        grpc::Status close_process(grpc::ServerContext*, const DfpData* dfp_data, Ping* reply) override{
            for(int i=0; i<dfp_data->group_id_size();++i){
                int group_id = dfp_data->group_id(i);
                std::string dfp_bytes_str =  dfp_data->dfp_bytes();
                const uint8_t* dfp_bytes = (uint8_t*)dfp_bytes_str.data();
                Dfp::DfpObject dfp_obj(dfp_bytes);
                if( device_dfp_map.at(group_id) !=dfp_obj.get_dfp_meta().hardware_hash){
                    reply->set_msg("close process called when the dfp was not connected");
                    reply->set_recv(false);
                    return grpc::Status::OK;
                }
                device_process_counter[group_id]--;
                if(device_process_counter[group_id]==0){
                    device_dfp_map.erase(group_id);
                    device_manager->close_device(group_id);
                    (*device_locks)[group_id] = false;
                }
            }
            delete uuid_dfp_map[dfp_data->uuid()];
            uuid_dfp_map.erase(dfp_data->uuid());
            uuid_time_map.erase(dfp_data->uuid());
            reply->set_recv(true);
            return grpc::Status::OK;
        }

        /**
         * Unlocks the specified device group ID, stopping the local heartbeat and joining the thread.
         * Only unlocks if the device is currently locked.
         */
        grpc::Status unlock(grpc::ServerContext*, const LockData* grp_id, Ping* reply) override{
            if((*device_locks)[grp_id->group_id()]){
                local_run_flags[grp_id->group_id()] = false;
                local_threads[grp_id->group_id()]->join();
                delete local_threads[grp_id->group_id()];
                local_threads[grp_id->group_id()] = NULL;
                (*device_locks)[grp_id->group_id()] = false;
            }
            reply->set_recv(true);
            return grpc::Status::OK;
        }

        /**
         * Heartbeat function to maintain the connection with the shared client.
         * Stores the current timestamp in uuid_time_map against the provided UUID.
        */
        grpc::Status heartbeat(grpc::ServerContext*, const Uuid* uuid, Ping* reply) override{
            uuid_time_map[uuid->id()]  = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
            reply->set_recv(true);
            return grpc::Status::OK;
        }

        /**
         * Heartbeat function to maintain the connection with the local client.
         * Stores the current timestamp in local_times against the provided group ID.
         */
        grpc::Status local_heartbeat(grpc::ServerContext*, const LockData* input, Ping* reply) override{
            local_times[input->group_id()]  = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
            reply->set_recv(true);
            return grpc::Status::OK;
        }

    private:
        std::unordered_map<int,uint64_t> device_dfp_map;
        std::unordered_map<uint64_t,int> checksum_tag_map;
        std::unordered_map<int,int> device_process_counter;
        MX::Runtime::DeviceManager* device_manager = NULL;
        bool setup_status;
        int dfp_tag_total=0;
        MX::Utils::fifo_queue<std::string> stream_queue;
        std::unordered_map<std::string,std::chrono::milliseconds> uuid_time_map;
        std::unordered_map<std::string,DfpData*> uuid_dfp_map;
        std::thread* heartbeat_thread;
        std::thread* local_heartbeat_thread;
        std::vector<bool> local_run_flags;
        std::vector<std::chrono::milliseconds> local_times;
        std::vector<std::thread*> local_threads;
        std::vector<atomic_bool> *device_locks;
        
        /**
         * Function to monitor the heartbeat of local clients given group ID. Checks
         * if the last heartbeat timestamp is more than 1 second ago and if so,
         * sets the local_run_flags to false and releases the lock for the group
         * ID. Sleeps for 500ms between each check.
         */
        void local_heartbeat_check(int group_id){
            while (local_run_flags[group_id]){
                std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) - local_times[group_id];
                if(duration>1s){
                    local_run_flags[group_id] = false;
                    (*device_locks)[group_id] = false;
                }
                std::this_thread::sleep_for(500ms);
            }
        }

        /**
         * Monitors the heartbeat of a client given a UUID. Checks if the last 
         * heartbeat timestamp is more than 1 second ago and if so, deletes the 
         * associated process and releases the lock for the group ID. Sleeps for 
         * 500ms between each check.
         */
        void heartbeat_check(){
            while (!server_shutdown_flag.load())
            {
                for(auto it: uuid_time_map){
                    std::chrono::milliseconds duration =
                        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) - it.second;
                    if(duration>1s){
                        DfpData* dfp_data = uuid_dfp_map[it.first];
                        std::string dfp_bytes_str =  dfp_data->dfp_bytes();
                        const uint8_t* dfp_bytes = (uint8_t*)dfp_bytes_str.data();
                        Dfp::DfpObject dfp_obj(dfp_bytes);
                        size_t max_out_size = 0;
                        for(int i=0; i<dfp_obj.get_dfp_meta().num_used_outports; ++i){
                            auto out_info = dfp_obj.output_port(i);
                            max_out_size = max(max_out_size,out_info->total_size);
                        }
                        uint8_t* temp_blob = new uint8_t[max_out_size*sizeof(float)];
                        Dfp::DfpMeta cur_meta = dfp_obj.get_dfp_meta();
                        int num_models = cur_meta.num_models;
                        for(int i=0; i<dfp_data->group_id_size();++i){
                            int group_id = dfp_data->group_id(i);
                            device_process_counter[group_id]--;
                            int total = 0;
                            std::unordered_map<std::string,int> model_map;
                            {
                                std::lock_guard uulock(uuid_q[group_id].m_mutex);
                                for(int i=0;i<num_models;++i)
                                model_map[it.first+"model"+to_string(i)] = i;

                                //Finding total number of ifmaps sent for this model
                                for(int i=0; i< (int) uuid_q[group_id].m_queue.size();++i){
                                    if(model_map.find(uuid_q[group_id].m_queue[i].first)!=model_map.end()){
                                        total++;
                                    }
                                }
                            }
                            while(total){
                                //Flushing all the sent ifmaps so that the MPU doesn't get stuck
                                for(auto it: model_map){
                                    std::optional<std::pair<std::string,int>> uuid_op = uuid_q[group_id].ifPophold(std::make_pair(it.first,-1));
                                    if(uuid_op.has_value()){
                                        auto ctx_pair = uuid_op.value();
                                        for(auto i : cur_meta.model_outports[it.second]){
                                            try
                                            {
                                                memx_status status = memx_stream_ofmap( ctx_pair.second, i, temp_blob, 0);
                                                if(memx_status_error(status)){
                                                    uuid_q[group_id].m_mutex.unlock();
                                                    std::cerr<<"error in flushing with code: "<<status<<" and count"<<total<<std::endl;
                                                }
                                            }
                                            catch(...)
                                            {
                                                uuid_q[group_id].m_mutex.unlock();
                                                std::cerr <<"flushing ofmap failed with exception" << '\n';
                                            }
                                        }
                                        total-=1;
                                    }
                                    uuid_q[group_id].m_mutex.unlock();
                                }
                            }
                            if(device_process_counter[group_id]==0){
                                device_dfp_map.erase(group_id);
                                device_manager->close_device(group_id);
                                (*device_locks)[group_id] = false;
                            }
                        }
                        delete [] temp_blob;
                        temp_blob = NULL;
                        delete dfp_data;
                        uuid_dfp_map.erase(it.first);
                        uuid_time_map.erase(it.first);
                        break;
                    }
                }
                std::this_thread::sleep_for(500ms); //Frequency at which heartbeat is checked
            }
        }
};

class OfmapCQServerImpl final {
 public:
  ~OfmapCQServerImpl() {
    server_->Shutdown();
    // Always shutdown the completion queue after the server.
    // Shutdown all completion queues
    for (auto& cq : cqs_) {
        cq->Shutdown();
    }
    
    // Wait for all CQ threads to finish
    for (auto& thread : cq_threads_) {
        thread.join();
    }
  }
  MxService::AsyncService service_;
  std::vector<std::unique_ptr<grpc::ServerCompletionQueue>> cqs_;
  std::vector<std::thread> cq_threads_;
  std::unique_ptr<grpc::Server> server_;

   /**
   Handles incoming RPCs on the given completion queue index.
  
   This function is meant to be run in a separate thread and is responsible
   for handling incoming RPCs on the given completion queue index. It will
   loop until the server_shutdown_flag is set, at which point it will exit.
  
   The function will delete any CallData objects that were created to handle
   RPCs, but were not completed before the server was shut down.
   */
  void HandleRpcs(int cq_index) {
    new CallData(&service_, cqs_[cq_index].get());
    void* tag; 
    bool ok;
    while (!server_shutdown_flag.load()) {
      if(!cqs_[cq_index]->Next(&tag, &ok)){
        break;
      }
      static_cast<CallData*>(tag)->Proceed();
    }
    while (cqs_[cq_index]->Next(&tag, &ok)) {
        //Leftover CallData objects at shutdown
       delete static_cast<CallData*>(tag);
    }
  }
  
  private:

  class CallData {
   public:
    /**
     A class that encapsulates a single RPC call and handles the various
     states it can be in. Each instance of this class is responsible for
     handling a single RPC call. It is created when a new request is
     received and deleted when the RPC call is finished.

     The class will automatically invoke the serving logic the first time
     Proceed() is called. All subsequent calls to Proceed() will be handled
     according to the current state of the object.

     The state transitions are as follows:

     - CREATE: The initial state of a CallData object. In this state, the
       object will call the serving logic to obtain a request and start
       processing it. The object will automatically transition to the
       PROCESS state after the request is processed.

     - PROCESS: The object is currently processing a request. If Proceed()
       is called in this state, the object will call the serving logic to
       obtain the result of the request and transition to the either WRITE orFINISH state.

     - WRITE: The object is currently writing the result of the request to
       the client. If Proceed() is called in this state, the object will keep writing
       until all the feature maps have been written. Then it will transition to the
       FINISH state.
       
     - FINISH: The object is currently finishing up a request. If Proceed()
       is called in this state, the object will call the serving logic to
       respond to the client and delete itself.
     */
    CallData(MxService::AsyncService* service, grpc::ServerCompletionQueue* cq)
        : service_(service), cq_(cq), status_(CREATE),writer_(&ctx_) {
      // Invoke the serving logic right away.
      Proceed();
    }

    void Proceed() {
      if (status_ == CREATE) {
        status_ = PROCESS;

        service_->Requestrecevice_ofmap(&ctx_,&request_,&writer_,cq_,cq_,this);
      } else if (status_ == PROCESS) {
        new CallData(service_, cq_);

        auto meta = ctx_.client_metadata();
        remaining_responses_ = request_.port_id_size();
        cur_idx_ = 0;
        if(meta.find("uuid")==meta.end()){
            return;
        }
        grpc::string_ref uuid_ref = meta.find("uuid")->second;
        std::string uuid(uuid_ref.data(),uuid_ref.size());
        reply_list_.resize(request_.port_id_size());
        for(int i=0; i<request_.port_id_size();++i){
            reply_list_[i].mutable_fmap()->resize(request_.size(i));
        }
        int group_id = request_.ctx_id()/2; //Currenly only one context is used in a group. Needs to be changed in future if two contexts are used in a group
        std::optional<std::pair<std::string,int>> uuid_op = uuid_q[group_id].ifPophold(std::make_pair(uuid,-1));

        //This condition is satisfied when the left uuid in uuid_q matheches with the uuid in the request
        //This means the current output feature map from the MPU belongs to this client
        if(uuid_op.has_value()){
            {
                //Notify the ifmap thread to take furthter requests
                std::unique_lock lock(queue_mutex[group_id]);
                queue_cv[group_id].notify_one();
            }
			//Copy the data from the ofmap buffer to the reply list which will be sent to the client
            for(int i=0; i<request_.port_id_size();++i){
                char* of_data = reply_list_[i].mutable_fmap()->data();
                try
                {
                    memx_status status = memx_stream_ofmap(request_.ctx_id(),request_.port_id(i),(void*)of_data,0);
                    if(memx_status_error(status)){
                        uuid_q[group_id].m_mutex.unlock();
                        std::cerr << "exception occured in driver stream ofmap" << '\n';
                        status_ = FINISH;
                        reply_list_[0].set_ctx_id(10001);
                        writer_.Finish(grpc::Status::CANCELLED,this);
                    }
                }
                catch(...)
                {
                    std::cerr << "exception occured in driver stream ofmap" << '\n';
                    uuid_q[group_id].m_mutex.unlock();
                    status_ = FINISH;
                    reply_list_[0].set_ctx_id(10001); //Error code
                    writer_.Finish(grpc::Status::CANCELLED,this);
                }
            }
            uuid_q[group_id].m_mutex.unlock();
            status_ = WRITE;
			//Start streaming the feature maps to the client
            writer_.Write(reply_list_[cur_idx_],this);
            remaining_responses_--;
            cur_idx_++;
        }
        else{
            uuid_q[group_id].m_mutex.unlock();
            status_ = FINISH;
            reply_list_[0].set_ctx_id(10000); //Code for uuid mismatch and telling client to retry
            writer_.WriteAndFinish(reply_list_[0],grpc::WriteOptions(),grpc::Status(grpc::StatusCode::NOT_FOUND, "Top of the queue not equal"),this);
        }
      }
      else if (status_ == WRITE) {
        if (remaining_responses_ > 0) {
            writer_.Write(reply_list_[cur_idx_],this);
            remaining_responses_--;
            cur_idx_++;
        } else {
            status_ = FINISH;
            writer_.Finish(grpc::Status::OK, this);
        }
      } 
      else {
        CHECK_EQ(status_, FINISH);
        delete this;
      }
    };

   private:
    // The means of communication with the gRPC runtime for an asynchronous
    // server.
    MxService::AsyncService* service_;
    // The producer-consumer queue where for asynchronous server notifications.
    grpc::ServerCompletionQueue* cq_;
    // Context for the rpc, allowing to tweak aspects of it such as the use
    // of compression, authentication, as well as to send metadata back to the
    // client.
    grpc::ServerContext ctx_;

    // What we get from the client.
    mxstream::OfPorts request_;
    // What we send back to the client.
    std::vector<mxstream::MxData> reply_list_;

    // Let's implement a tiny state machine with the following states.
    enum CallStatus { CREATE, PROCESS, FINISH, WRITE };
    CallStatus status_;  // The current serving state.
    grpc::ServerAsyncWriter<mxstream::MxData> writer_;
    int remaining_responses_;
    int cur_idx_;
  };
};
class MxServiceAsyncImpl final : public MxService::CallbackService {
 public:
    /**
     * This function is used by the gRPC framework to handle the ifmap rpc
     * from the client.  It reads the ifmap data from the client, and sends 
	 * it ot MPU for inference. It then sends the response back to the client
	 * for successful or failed operation
     */
    grpc::ServerReadReactor<MxData>* send_ifmap(grpc::CallbackServerContext* context,
                                              Ping* response) override {
        class fmapReader: public grpc::ServerReadReactor<MxData> {
            public:
                fmapReader(grpc::CallbackServerContext* context,Ping* response):
                                                        response_{response},
                                                        context_{context}
                {
                    MxData fmap;
                    auto meta = context->client_metadata();
                    grpc::string_ref uuid_ref = meta.find("uuid")->second;
                    std::string uuid_(uuid_ref.data(),uuid_ref.size());
                    new_uuid = strdup(uuid_.c_str());
                    response_->set_recv(true);
                    idx_.store(0);
                    ifmap_.push_back(new MxData);
                    StartRead(ifmap_.at(idx_.load()));
                }

                ~fmapReader(){
                    free(new_uuid);
                }

                void OnReadDone(bool ok) override {
                    if (ok) {
                        idx_++;
                        ifmap_.push_back(new MxData);
                        StartRead(ifmap_.at(idx_.load()));                      
                    }
                    else{
                        if(context_->IsCancelled()){
                            printf("ifmap cancelled\n");
                            Finish(grpc::Status::CANCELLED);
                            return;
                        }
                        int ctx_id = ifmap_.at(0)->ctx_id();
                        int group_id = ctx_id/2;
						//Cap the queue size to 30 so that new clients don't need to wait too long(Can be removed if driver queue is enough)
                        while(uuid_q[group_id].size()>30){
                            std::unique_lock lock(queue_mutex[group_id]);
                            queue_cv[group_id].wait(lock);
                        }
                        {
                            std::lock_guard lock(send_mutex[group_id]);
                            for(int i=0; i<static_cast<int>(ifmap_.size())-1;++i){
                                MxData* fmap = ifmap_.at(i);
                                memx_status status = memx_stream_ifmap(fmap->ctx_id(),fmap->port_id(),(void*)fmap->mutable_fmap()->data(),0);
                                if memx_status_error(status){
                                    response_->set_msg("Error in stream_ifmap");
                                    response_->set_recv(false);
                                }
                                ctx_id = fmap->ctx_id();
                            }
							//Push the uuid of the latest ifmap sent to maintian the order for ofmap
                            uuid_q[group_id].push(std::make_pair(std::string(new_uuid),ctx_id)); 
                        }
                        for(int i=0; i<static_cast<int>(ifmap_.size());++i){
                            delete ifmap_.at(i);
                        }
                        Finish(grpc::Status::OK);
                    }                        
                }
                void OnDone() override {
                    delete this;
                }
            private:
                vector<MxData*> ifmap_;
                Ping* response_;
                atomic_int idx_;
                char* new_uuid;
                grpc::CallbackServerContext* context_;
        }; 
        return new fmapReader(context,response);                                         
    }
};

/**
 * Starts a synchronous gRPC server on the specified address.
 * 
 * The server is configured with no compression and a maximum receive message size
 * of 100 MB. It uses an instance of MxServiceImpl to handle incoming RPCs.
 * Once started, the server will wait indefinitely for incoming requests.
 */

void RunSyncServer(std::string server_address){
  MxServiceImpl service;
  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  builder.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_NONE);
  builder.SetMaxReceiveMessageSize(1024*1024*100);

  sync_server = builder.BuildAndStart();
  sync_server->Wait();
}

/**
 * Starts asynchronous gRPC servers for sening and receiving on the specified address and ports.
 */
void RunServers(uint16_t port, std::string listen_address) {
  std::string server_address = absl::StrFormat("%s:%d", listen_address.c_str(), port);
  std::string server_address_send = absl::StrFormat("%s:%d", listen_address.c_str(), port+1);
  std::string server_address_recv = absl::StrFormat("%s:%d", listen_address.c_str(), port+2);
  MxServiceAsyncImpl async_service;

  grpc::ServerBuilder builder_send;
  builder_send.AddListeningPort(server_address_send, grpc::InsecureServerCredentials());
  builder_send.RegisterService(&async_service);
  builder_send.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_NONE);
  std::thread sync_thread(RunSyncServer,server_address);

  grpc::ServerBuilder builder_recv;
  OfmapCQServerImpl cq_server;
  builder_recv.AddListeningPort(server_address_recv, grpc::InsecureServerCredentials());
  builder_recv.RegisterService(&cq_server.service_);

  for (int i = 0; i < NUM_OFMAP_CQS; ++i) {
      cq_server.cqs_.emplace_back(builder_recv.AddCompletionQueue());
  }
  
  cq_server.server_ = builder_recv.BuildAndStart();
  
  // Start handling requests on each CQ in separate threads
  for (int i = 0; i < NUM_OFMAP_CQS; ++i) {
       cq_server.cq_threads_.push_back(std::thread([&cq_server, i]() {
           cq_server.HandleRpcs(i);
       }));
  }
  builder_recv.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_NONE);

  std::unique_ptr<grpc::Server> server_send(builder_send.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  std::cout << "Server listening on " << server_address_send << std::endl;
  std::cout << "Server listening on " << server_address_recv << std::endl;
  sync_thread.join();
}

/**
 * Reads a configuration file and parses it into a listen address and base port.
 * The configuration file is expected to be in the format:
 * LISTEN_ADDRESS=<listen address>
 * BASE_PORT=<base port>
 * 
 * If the file does not exist or is malformed, the function will return a default
 * value of 127.0.0.1 for the listen address and 10000 for the base port.
 */
std::string parse_server_config(uint16_t *port){

  #ifdef __linux__
    // File path of the configuration file
    const std::string filePath = "/etc/memryx/mxa_manager.conf";

    // Variables to hold parsed data
    std::string listenAddress;
    uint16_t basePort;
    
    std::string line;
    std::unordered_map<std::string, std::string> configMap;

    // Open the file
    std::ifstream file(filePath);
    if (!file.is_open()) {
        goto failed;
    }

    // Parse the file line by line
    while (std::getline(file, line)) {
        // Remove leading and trailing whitespaces
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;

        // Split the line into key and value
        auto delimiterPos = line.find('=');
        if (delimiterPos == std::string::npos) {
            goto failed;
        }

        std::string key = line.substr(0, delimiterPos);
        std::string value = line.substr(delimiterPos + 1);

        // Remove optional quotes around the value
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }

        // Store in the map
        configMap[key] = value;
    }

    file.close();

    // Validate and convert values
    try {
        if (configMap.find("LISTEN_ADDRESS") != configMap.end()) {
            listenAddress = configMap["LISTEN_ADDRESS"];
        } else {
            goto failed;
        }

        if (configMap.find("BASE_PORT") != configMap.end()) {
            basePort = static_cast<uint16_t>(std::stoi(configMap["BASE_PORT"]));
        } else {
            goto failed;
        }
    } catch (const std::exception &e) {
        goto failed;
    }

    *port = basePort;
    return listenAddress;


  failed:
#endif

    *port = 10000;
    return std::string("127.0.0.1");
}


int main(){
    server_shutdown_flag.store(false);
    std::signal(SIGINT, signalHandler);

    uint16_t port = 0;
    std::string listen_addr = parse_server_config(&port);

    RunServers(port, listen_addr);

}
