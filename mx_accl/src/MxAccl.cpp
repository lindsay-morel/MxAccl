#include <memx/accl/MxAccl.h>
#include <sstream>
#include <uuid/uuid.h>
#include <grpcpp/grpcpp.h>
#include "mx_proc.grpc.pb.h"

using namespace MX::Runtime;
using namespace MX::Types;
using namespace MX::Utils;
using mxstream::MxService;
using mxstream::MxData;

struct daemon_items{
    std::shared_ptr<grpc::Channel> grpc_channel_;
    std::shared_ptr<mxstream::MxService::Stub> stub_;
    std::shared_ptr<mxstream::MxService::Stub> model_stub_send_;
    std::shared_ptr<mxstream::MxService::Stub> model_stub_recv_;
    mxstream::Uuid uuid_;
    mxstream::DfpData grpc_dfp;
};

MxAccl::MxAccl(bool use_shared_mode, std::string server_ip, unsigned int server_port_base){
    dfp_valid = false;
    setup_status = false;
    grpc::ChannelArguments sync_ch_args;
    grpc::ChannelArguments async_ch_args;
    sync_ch_args.SetMaxSendMessageSize(1024*1024*100);
    sync_ch_args.SetCompressionAlgorithm(GRPC_COMPRESS_NONE);
    async_ch_args.SetCompressionAlgorithm(GRPC_COMPRESS_NONE);
    std::string base_port = std::to_string(server_port_base);
    std::string base_plus1 = std::to_string(server_port_base+1);
    std::string base_plus2 = std::to_string(server_port_base+2);
    daemon_items_ = std::make_unique<daemon_items>();
    if(!use_shared_mode){
        daemon_items_->grpc_channel_ = grpc::CreateCustomChannel(server_ip+":"+base_port, grpc::InsecureChannelCredentials(),async_ch_args);
        daemon_items_->stub_ = MxService::NewStub(daemon_items_->grpc_channel_);
        device_manager = new MX::Runtime::DeviceManager(daemon_items_->stub_.get());
    }
    else{
        device_manager = NULL;
        daemon_items_->grpc_channel_ = grpc::CreateCustomChannel(server_ip+":"+base_port, grpc::InsecureChannelCredentials(),sync_ch_args);
        daemon_items_->stub_ = MxService::NewStub(daemon_items_->grpc_channel_);
        daemon_items_->model_stub_send_ = MxService::NewStub(grpc::CreateCustomChannel(server_ip+":"+base_plus1, grpc::InsecureChannelCredentials(),async_ch_args));
        daemon_items_->model_stub_recv_ = MxService::NewStub(grpc::CreateCustomChannel(server_ip+":"+base_plus2, grpc::InsecureChannelCredentials(),async_ch_args));
    }
    // device_manager->print_available_devices();

}

int MxAccl::connect_dfp(const std::filesystem::path pdfp_path,int group_id){
    std::vector<int> devices_to_use = {group_id};
    return connect_dfp(pdfp_path,devices_to_use);
}

int MxAccl::connect_dfp(const uint8_t *dfp_bytes,int group_id){
    std::vector<int> devices_to_use = {group_id};
    return connect_dfp(dfp_bytes,devices_to_use);
}

int MxAccl::connect_dfp(const std::filesystem::path pdfp_path,std::vector<int>& device_ids_to_use){
    if(dfp_valid){
        throw std::runtime_error("Only one dfp allowed per Accl object");
    }

    if(device_ids_to_use.empty()){
        throw runtime_error("device_ids_to_use parameter cannot be empty");
    }

    dfp_path = pdfp_path;
    dfp_tag = 0;
    if(device_manager!=NULL){
        device_manager->opendfp(dfp_path, dfp_tag);
    }
    return connect_dfp((uint8_t*)NULL,device_ids_to_use);
}

int MxAccl::connect_dfp(const uint8_t *dfp_bytes, std::vector<int>& device_ids_to_use){
    if(dfp_valid){
        throw std::runtime_error("Only one dfp allowed per Accl object");
    }

    if(device_ids_to_use.empty()){
        throw runtime_error("device_ids_to_use parameter cannot be empty");
    }

    device_ids_ = device_ids_to_use;
    if(device_manager == NULL){
        uuid_t uuid;
        uuid_generate_time_safe(uuid);
        uuid_unparse(uuid,uuid_str);
        if(dfp_bytes ==NULL){
            dfp_ = new Dfp::DfpObject(dfp_path.string().c_str());
        }
        else{
            dfp_ = new Dfp::DfpObject(dfp_bytes);
        }
        num_models_ = dfp_->get_dfp_meta().num_models;
        for(int i=0; i<num_models_;++i){
            std::string s(uuid_str);
            s+=("model"+to_string(i));
            models_uuid.push_back(s);
        }
        num_chips_ = dfp_->get_dfp_meta().num_chips;
        dfp_valid = dfp_->valid;
        grpc::ClientContext ctx;
        for(uint32_t id : device_ids_to_use){
            daemon_items_->grpc_dfp.add_group_id(id);
        }
        daemon_items_->grpc_dfp.set_dfp_bytes(dfp_->src_dfp_bytes,dfp_->dfp_byte_size);
        daemon_items_->grpc_dfp.set_uuid(uuid_str,36);
        mxstream::Ping reply;
        grpc::Status status = daemon_items_->stub_->connect_dfp(&ctx,daemon_items_->grpc_dfp,&reply);
        if(!status.ok()){
            throw std::runtime_error("server connection for connect_dfp failed with: "+status.error_message());
        }
        if(!reply.recv()){
            throw std::runtime_error(reply.msg());
        }
        setup_status = true;
        this->init_mx_models(device_ids_to_use);
        heartbeat_run.store(true);
        heartbeat_thread = new std::thread(&MxAccl::heartbeat_fun,this);
        return 0;
    }

    dfp_tag = 0;
    if(dfp_bytes!=NULL){
        dfp_path = std::filesystem::path("<BYTES>");
        device_manager->opendfp_bytes(dfp_bytes, dfp_tag);
    }

    run.store(false);
    dfp_valid = device_manager->get_dfp_validity(dfp_tag);
    if(!dfp_valid){
        throw runtime_error("Cannot parse dfp file - Please check given dfp");
    }

    mx_checkandthrow(device_manager->setup_mxa(dfp_tag, device_ids_to_use));
    setup_status = true;
    local_heartbeat_thread = new std::thread(&MxAccl::local_heartbeat_fun,this);
    local_heartbeat_run.store(true);
    mx_checkandthrow(device_manager->attach_dfp_to_device(dfp_tag));
    mx_checkandthrow(device_manager->download_dfp_to_device(dfp_tag));
    mx_checkandthrow(device_manager->init_mx_models(dfp_tag, &models));
    num_models_ =  device_manager->get_dfp_num_models(dfp_tag);
    num_chips_ = device_manager->get_dfp_num_chips(dfp_tag);
    return dfp_tag;
}


void MxAccl::start(){
    if (dfp_valid)
    {
        // int num_models =  device_manager->get_dfp_num_models(dfp_tag);

        if(get_num_streams() == 0){
            throw logic_error("accl start called before connect_stream for auto threading");
        }
        else{
            // set run status to true
            run.store(true);
            for (int i = 0; i < num_models_; ++i)
            {
                //start models that have a stream connected to it
                if(models[i]->get_num_streams()>0)
                    models[i]->model_start();
            }
        }
    }
    else
    {
        printf("MxAccl::start(): ERROR, valid==false. Did initialization fail?");
    }
}

void MxAccl::wait(){
    if(dfp_valid){
        // int num_models =  device_manager->get_dfp_num_models(dfp_tag);
        for (int i = 0; i < num_models_; ++i)
        {
            //wait for the all the models to finish streaming
            if(models[i]->get_num_streams()>0)
                models[i]->model_wait();
        }
    }
}

void MxAccl::stop()
{

    if (dfp_valid && run.load())
    {
        // int num_models =  device_manager->get_dfp_num_models(dfp_tag);
        for (int i = 0; i < num_models_; ++i)
        {   
            //Stop all models
            if(models[i]->get_num_streams()>0)
                models[i]->model_stop();
        }
        run.store(false);
    }
}

void MxAccl::connect_stream(float_callback_t in_cb, float_callback_t out_cb, int stream_id, int model_id, int dfp_id){
    //!!!!TODO: Need to use dfp_id for future
    if(dfp_id!=0){
        throw std::runtime_error("only one dfp per MxAccl allowed");
    }
    models[model_id]->connect_stream(in_cb,out_cb,stream_id);
}

// void MxAccl::connect_stream(int_callback_t in_cb, float_callback_t out_cb, int stream_id, int model_id){
//     models[model_id]->connect_stream(in_cb,out_cb,stream_id);
// }

void MxAccl::connect_post_model(std::filesystem::path post_model_path, int model_idx, const std::vector<size_t>& post_size_list){
    models[model_idx]->model_set_post(post_model_path,post_size_list);
}

void MxAccl::connect_pre_model(std::filesystem::path pre_model_path, int model_idx){
    models[model_idx]->model_set_pre(pre_model_path);
}

void MxAccl::init_mx_models(std::vector<int>& device_ids_to_use){

    for (int i = 0; i < num_models_; ++i)
    {

        vector<uint8_t> in_ports = dfp_->get_dfp_meta().model_inports[i];
        uint8_t format = dfp_->input_port(in_ports[0])->format;
        for(auto device_id: device_ids_to_use){
            context_ids_vector_.push_back(device_id*2);
        }
        if(format == MX_FMT_RGB888){
            // MxModel<uint8_t> *im = new MxModel<uint8_t>(i, dfp_mxa_map.at(dfp_tag).dfp, &dfp_mxa_map.at(dfp_tag).context_ids_vector);
            // mxmodel_vector->push_back(im); 
            throw(std::runtime_error("int inputs are currently not supported"));               
        }
        else{
            MxModel<float> *fm = new MxModel<float>(i, dfp_,&context_ids_vector_,daemon_items_->model_stub_send_.get(),daemon_items_->model_stub_recv_.get(),models_uuid[i]);
            models.push_back(fm);
        }
    }
}

MxAccl::~MxAccl()
{
    //if destructor is called without calling stop, need to exit gracefully
    if(run.load()){
        this->stop();
    }
    //Close the MXA
    if(dfp_valid && setup_status){
        for (int i = 0; i < num_models_; ++i)
        {
            //delete all the models created
            delete models[i];
        }
        models.clear();
        if(device_manager){
            device_manager->cleanup__all_dfps();
            mx_checkandprint(device_manager->close_all_devices());
            local_heartbeat_run.store(false);
            local_heartbeat_thread->join();
            delete local_heartbeat_thread;
            local_heartbeat_thread = NULL;
        }
        else{
            grpc::ClientContext ctx;
            mxstream::Ping response;
            heartbeat_run.store(false);
            heartbeat_thread->join();
            grpc::Status status = daemon_items_->stub_->close_process(&ctx,daemon_items_->grpc_dfp,&response);
            if(!status.ok()){
                std::cerr<<response.msg()<<"\n";
            }
            delete heartbeat_thread;
            heartbeat_thread = NULL;
        }
    }

    
    // device_manager->cleanup_all_setup_maps();
    //delete the DFP object
    if(device_manager!=NULL){
        // if(we_opened_dfp){
        delete device_manager;
        device_manager = NULL;
    }
    if(dfp_!=NULL){
        delete dfp_;
        dfp_ = NULL;
    }
}

int MxAccl::get_num_models(){
    if(!dfp_valid){
        return 0;
    }
    return  num_models_;
}

int MxAccl::get_num_streams(){

    int ans = 0;
    if(!dfp_valid){
        return ans;
    }
    // int num_models =  device_manager->get_dfp_num_models(dfp_tag);
    for(int i =0; i<num_models_; ++i){
        ans+=models[i]->get_num_streams();
    }
    return ans;
}

int MxAccl::get_dfp_num_chips(){
    if(!dfp_valid){
        throw std::runtime_error("dfp is not connected.");
    }
    return  num_chips_;
}

MX::Types::MxModelInfo MxAccl::get_model_info(int model_id) const{
    if(model_id>= static_cast<int>(models.size())){
        std::ostringstream oss;
        int num_models = models.size();
        oss << "Invalid model ID passed : Number of models available = "<<num_models<<"\n model_id range is 0 to "<<num_models-1;
        throw runtime_error(oss.str());
    }
    else{
        return models[model_id]->return_model_info();
    }
}

MX::Types::MxModelInfo MxAccl::get_pre_model_info(int model_id) const{
    if(model_id>= static_cast<int>(models.size())){
        std::ostringstream oss;
        int num_models = models.size();
        oss << "Invalid model ID passed : Number of models available = "<<num_models<<"\n model_id range is 0 to "<<num_models-1;
        throw runtime_error(oss.str());
    }
    else{
        return models[model_id]->return_pre_model_info();
    }
}

MX::Types::MxModelInfo MxAccl::get_post_model_info(int model_id) const{
    if(model_id>= static_cast<int>(models.size())){
        std::ostringstream oss;
        int num_models = models.size();
        oss << "Invalid model ID passed : Number of models available = "<<num_models<<"\n model_id range is 0 to "<<num_models-1;
        throw runtime_error(oss.str());
    }
    else{
        return models[model_id]->return_post_model_info();
    }
}


void MxAccl::set_num_workers(int input_num_workers, int output_num_workers, int model_idx){
    if(model_idx>= static_cast<int>(models.size())){
        std::ostringstream oss;
        int num_models = models.size();
        oss << "Invalid model ID passed : Number of models available = "<<num_models<<"\n model_id range is 0 to "<<num_models-1;
        throw runtime_error(oss.str());
    }
    models[model_idx]->set_num_workers(input_num_workers,output_num_workers);
}

void MxAccl::set_parallel_fmap_convert(int num_threads, int model_idx){
    if(model_idx>= static_cast<int>(models.size())){
        std::ostringstream oss;
        int num_models = models.size();
        oss << "Invalid model ID passed : Number of models available = "<<num_models<<"\n model_id range is 0 to "<<num_models-1;
        throw runtime_error(oss.str());
    }
    models[model_idx]->set_parallel_fmap_convert(num_threads);
}

void MxAccl::heartbeat_fun(){
    while (heartbeat_run.load())
    {
        grpc::ClientContext ctx;
        daemon_items_->uuid_.set_id(uuid_str,36);
        mxstream::Ping response;
        grpc::Status status = daemon_items_->stub_->heartbeat(&ctx,daemon_items_->uuid_,&response);
        if(!status.ok()){
            throw std::runtime_error("heartbeat failed with: "+status.error_message());
        }
        std::this_thread::sleep_for(250ms);   
    }
}

void MxAccl::local_heartbeat_fun(){
    while (local_heartbeat_run.load())
    {
        grpc::ClientContext ctx;
        for(int i = 0; i < static_cast<int>(device_ids_.size()); ++i){
            mxstream::LockData lck_data;
            lck_data.set_group_id(i);
            mxstream::Ping response;
            grpc::Status status = daemon_items_->stub_->local_heartbeat(&ctx,lck_data,&response);
            if(!status.ok()){
                throw std::runtime_error("localheartbeat failed with: "+status.error_message());
            }
        }
        std::this_thread::sleep_for(250ms);   
    }
}

