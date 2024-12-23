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

#include "mx_proc.grpc.pb.h"
#include "mx_proc.pb.h"
#include <openssl/sha.h>

using mxstream::MxService;
using mxstream::LockData;
using mxstream::DfpData;
using mxstream::Ping;
using mxstream::MxData;
using mxstream::Uuid;

atomic_bool server_shutdown_flag;

MX::Utils::fifo_deque<std::string,int> uuid_q;
std::mutex send_mutex;
std::unique_ptr<grpc::Server> sync_server;

void signalHandler(int signum) {
    std::cout << "Signal " << signum << " shutting down the daemon" << std::endl;
    server_shutdown_flag.store(true);
    if(sync_server){
        sync_server->Shutdown();
    }
}

class MxServiceImpl final: public MxService::Service{
    public:
        MxServiceImpl(){
            heartbeat_thread = new std::thread(&MxServiceImpl::heartbeat_check,this);
            int all_devices_count;
            memx_status status = memx_operation_get_device_count(&all_devices_count);
            if memx_status_error(status){
                printf("daemon failed to get device count\n");
            }
            device_locks = new std::vector<atomic_bool>(all_devices_count);
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
                (*device_locks)[grp_id->group_id()] = true;
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

        grpc::Status connect_dfp(grpc::ServerContext*, const DfpData* dfp_data, Ping* reply) override{
            reply->set_recv(true);
            std::vector<int> device_ids_to_use;
            std::string dfp_bytes_str =  dfp_data->dfp_bytes();
            const uint8_t* dfp_bytes = (uint8_t*)dfp_bytes_str.data();
            Dfp::DfpObject dfp_obj(dfp_bytes);
            if(dfp_obj.get_dfp_meta().dfp_version <= 5){
                reply->set_recv(false);
                reply->set_msg("unsupported dfp version passed. Dfp version should be more than 6");
                return grpc::Status::OK;
            } 
            for(int i=0; i<dfp_data->group_id_size();++i){
                int group_id = dfp_data->group_id(i);
                if(device_dfp_map.find(group_id) != device_dfp_map.end()){
                    if( device_dfp_map.at(group_id) != dfp_obj.get_dfp_meta().hardware_hash){
                        reply->set_recv(false);
                        reply->set_msg("A process with a different dfp is active on group: "+std::to_string(group_id)+". Must use same dfp in multiple processes or wait for the other processes to end first");
                        return grpc::Status::OK;
                    }
                    device_process_counter[group_id]++;
                }
                else{
                    device_ids_to_use.push_back(group_id);
                }
            }

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

            if(device_manager==NULL){
                device_manager = new MX::Runtime::DeviceManager(NULL, true);
            }
            int dfp_tag = 0;
            if(checksum_tag_map.find(dfp_obj.get_dfp_meta().hardware_hash) == checksum_tag_map.end()){
                checksum_tag_map[dfp_obj.get_dfp_meta().hardware_hash] = dfp_tag_total;
                dfp_tag = dfp_tag_total;
                dfp_tag_total+=1;
            }
            else{
                dfp_tag = checksum_tag_map[dfp_obj.get_dfp_meta().hardware_hash];
            }


            mx_retval_t open_ret = device_manager->opendfp_bytes(dfp_bytes, dfp_tag);
            reply->set_recv(open_ret.error_flag);
            if(!open_ret.error_flag){
                reply->set_msg(open_ret.error_msg);
                return grpc::Status::OK;
            }

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


            mx_retval_t setup_ret =  device_manager->setup_mxa(dfp_tag, device_ids_to_use);
            if(!setup_ret.error_flag){
                reply->set_recv(false);
                reply->set_msg(setup_ret.error_msg);
                return grpc::Status::OK;
            }

            mx_retval_t attach_ret =  device_manager->attach_dfp_to_device(dfp_tag);
            if(!attach_ret.error_flag){
                reply->set_recv(false);
                reply->set_msg(attach_ret.error_msg);
                return grpc::Status::OK;
            }

            mx_retval_t download_ret =  device_manager->download_dfp_to_device(dfp_tag);
            if(!download_ret.error_flag){
                reply->set_recv(false);
                reply->set_msg(download_ret.error_msg);
                return grpc::Status::OK;
            }

            if(reply->recv()){
                for(int i=0; i<static_cast<int>(device_ids_to_use.size());++i){
                    int group_id = device_ids_to_use[i];
                    device_dfp_map[group_id] = dfp_obj.get_dfp_meta().hardware_hash;
                    device_process_counter[group_id] = 1;
                }
            }
            return grpc::Status::OK;
        }

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

        grpc::Status heartbeat(grpc::ServerContext*, const Uuid* uuid, Ping* reply) override{
            uuid_time_map[uuid->id()]  = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
            reply->set_recv(true);
            return grpc::Status::OK;
        }

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
                                std::lock_guard uulock(uuid_q.m_mutex);
                                for(int i=0;i<num_models;++i)
                                model_map[it.first+"model"+to_string(i)] = i;
                            
                                for(int i=0; i< (int) uuid_q.m_queue.size();++i){
                                    if(model_map.find(uuid_q.m_queue[i].first)!=model_map.end()){
                                        total++;
                                    }
                                }
                            }
                            while(total){
                                for(auto it: model_map){
                                    std::optional<std::pair<std::string,int>> uuid_op = uuid_q.ifPophold(std::make_pair(it.first,-1));
                                    if(uuid_op.has_value()){
                                        auto ctx_pair = uuid_op.value();
                                        for(auto i : cur_meta.model_outports[it.second]){
                                            try
                                            {
                                                memx_status status = memx_stream_ofmap( ctx_pair.second, i, temp_blob, 0);
                                                if(memx_status_error(status)){
                                                    uuid_q.m_mutex.unlock();
                                                    std::cerr<<"error in flushing with code: "<<status<<" and count"<<total<<std::endl;
                                                }
                                            }
                                            catch(...)
                                            {
                                                uuid_q.m_mutex.unlock();
                                                std::cerr <<"flushing ofmap failed with exception" << '\n';
                                            }
                                        }
                                        total-=1;
                                    }
                                    uuid_q.m_mutex.unlock();
                                }
                                std::this_thread::sleep_for(1ms);
                            }
                            printf("process deleted\n");
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
                std::this_thread::sleep_for(500ms);
            }
        }
};

class MxServiceAsyncImpl final : public MxService::CallbackService {
 public:
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
                        int ctx_id = 0;
                        {
                            std::lock_guard lock(send_mutex);
                            for(int i=0; i<static_cast<int>(ifmap_.size())-1;++i){
                                MxData* fmap = ifmap_.at(i);
                                memx_status status = memx_stream_ifmap(fmap->ctx_id(),fmap->port_id(),(void*)fmap->mutable_fmap()->data(),0);
                                if memx_status_error(status){
                                    response_->set_msg("Error in stream_ifmap");
                                    response_->set_recv(false);
                                }
                                ctx_id = fmap->ctx_id();
                            }
                            uuid_q.push(std::make_pair(std::string(new_uuid),ctx_id)); 
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

    grpc::ServerWriteReactor<MxData>* recevice_ofmap(grpc::CallbackServerContext* context, const mxstream::OfPorts* port_list) override{
        class fmapWriter : public grpc::ServerWriteReactor<MxData> {
            public:    
                fmapWriter(grpc::CallbackServerContext* context, const mxstream::OfPorts* port_list):
                                port_list_{port_list}
                {
                    auto meta = context->client_metadata();
                    grpc::string_ref uuid_ref = meta.find("uuid")->second;
                    std::string uuid(uuid_ref.data(),uuid_ref.size());
                    send_ofmap.resize(port_list_->port_id_size());
                    for(int i=0; i<port_list_->port_id_size();++i){
                        send_ofmap[i].mutable_fmap()->resize(port_list_->size(i));
                    }
                    while(true){
                        if(!context->IsCancelled()){
                            std::optional<std::pair<std::string,int>> uuid_op = uuid_q.ifPophold(std::make_pair(uuid,-1));
                            if(uuid_op.has_value()){
                                for(int i=0; i<port_list_->port_id_size();++i){
                                    char* of_data = send_ofmap[i].mutable_fmap()->data();
                                    try
                                    {
                                        memx_status status = memx_stream_ofmap(port_list_->ctx_id(),port_list_->port_id(i),(void*)of_data,0);
                                        if(memx_status_error(status)){
                                            uuid_q.m_mutex.unlock();
                                            Finish(grpc::Status(grpc::StatusCode::INTERNAL, "stream_ofmap error"));
                                            return;
                                        }
                                    }
                                    catch(...)
                                    {
                                        std::cerr << "exception occured in driver stream ofmap" << '\n';
                                        uuid_q.m_mutex.unlock();
                                    }
                                }
                                uuid_q.m_mutex.unlock();
                                break;
                            }
                            uuid_q.m_mutex.unlock();
                        }
                        else{
                            printf("ofmap cancelled\n");
                            break;
                        }
                        this_thread::sleep_for(750us);
                    }
                    idx_ = 0;
                    NextWrite(idx_.load());
                }
                void OnWriteDone(bool ok) override {
                    if (!ok) {
                        Finish(grpc::Status::OK);
                    }
                    else{
                        idx_++;
                        NextWrite(idx_.load());
                    }
                }
                void OnDone() override {
                    delete this;
                }
            private:
                void NextWrite(int idx){
                    if(idx >= port_list_->port_id_size()){
                        Finish(grpc::Status::OK);
                        return;
                    }
                    StartWrite(&send_ofmap[idx]);
                    return;
                }
                atomic_int idx_; 
                const mxstream::OfPorts* port_list_;
                std::vector<MxData> send_ofmap;
        };   

        return new fmapWriter(context,port_list);
    }
};

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

void RunServer(uint16_t port, std::string listen_address) {
  std::string server_address = absl::StrFormat("%s:%d", listen_address.c_str(), port);
  std::string server_address_send = absl::StrFormat("%s:%d", listen_address.c_str(), port+1);
  std::string server_address_recv = absl::StrFormat("%s:%d", listen_address.c_str(), port+2);
  MxServiceAsyncImpl async_service;

  grpc::ServerBuilder builder_send;
  builder_send.AddListeningPort(server_address_send, grpc::InsecureServerCredentials());
  builder_send.RegisterService(&async_service);
  builder_send.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_NONE);

  grpc::ServerBuilder builder_recv;
  builder_recv.AddListeningPort(server_address_recv, grpc::InsecureServerCredentials());
  builder_recv.RegisterService(&async_service);
  builder_recv.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_NONE);

  std::unique_ptr<grpc::Server> server_send(builder_send.BuildAndStart());
  std::unique_ptr<grpc::Server> server_recv(builder_recv.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  std::cout << "Server listening on " << server_address_send << std::endl;
  std::cout << "Server listening on " << server_address_recv << std::endl;

  std::thread sync_thread(RunSyncServer,server_address);
  sync_thread.join();
}

std::string parse_server_config(uint16_t *port){

  #ifdef __linux__
    // File path of the configuration file
    const std::string filePath = "/etc/memryx/mx_server.conf";

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

    RunServer(port, listen_addr);

}
