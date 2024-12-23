#include <unordered_map>
#include <memx/accl/DeviceManager.h>
#include <sstream>
#include <fstream>

#include <grpcpp/grpcpp.h>
#include "mx_proc.grpc.pb.h"

using namespace MX::Runtime;
using namespace MX::Types;
using namespace MX::Utils;
using namespace std;
using mxstream::MxService;
using mxstream::LockData;
using mxstream::Ping;

DeviceManager::DeviceManager(void* stub, bool server_mode){    
    all_devices_count = 0;
    required_devices = 0;

    server_mode_ = server_mode;
    stub_ = stub;
    // let' get all devices and manage them
    // this->get_available_devices();
}

mx_retval_t DeviceManager::opendfp_bytes(const uint8_t *b, int dfp_tag){

    dfp_rt_info ddi;
    ddi.dfp = new Dfp::DfpObject(b);
    Dfp::DfpMeta temp_meta = ddi.dfp->get_dfp_meta();

    ddi.is_bytes = true;
    ddi.dfp_filename_path = std::filesystem::path("<BYTES>");
    ddi.dfp_num_chips = temp_meta.num_chips;
    ddi.num_models = temp_meta.num_models;
    ddi.mxa_gen = temp_meta.mxa_gen;
    ddi.dfp_meta = temp_meta;
    ddi.use_multigroup_lb = temp_meta.use_multigroup_lb;
    ddi.context_ids_vector  = {};
    ddi.valid = ddi.dfp->valid;

    auto it = this->dfp_mxa_map.find(dfp_tag);
    if(it == this->dfp_mxa_map.end()){
        this->dfp_mxa_map.emplace(dfp_tag, std::move(ddi));
    }
    else{  
        it->second = ddi;
    }

    mx_retval_t ret(ddi.valid);
    return ret;
}

mx_retval_t DeviceManager::opendfp(const std::filesystem::path dfp_filename, int dfp_tag){

    dfp_rt_info ddi;
    ddi.dfp = new Dfp::DfpObject(dfp_filename.string().c_str());
    Dfp::DfpMeta temp_meta = ddi.dfp->get_dfp_meta();

    ddi.is_bytes = false;
    ddi.dfp_filename_path = dfp_filename;
    ddi.dfp_num_chips = temp_meta.num_chips;
    ddi.num_models = temp_meta.num_models;
    ddi.mxa_gen = temp_meta.mxa_gen;
    ddi.dfp_meta = temp_meta;
    ddi.use_multigroup_lb = temp_meta.use_multigroup_lb;
    ddi.context_ids_vector  = {};
    ddi.valid = ddi.dfp->valid;

    auto it = this->dfp_mxa_map.find(dfp_tag);
    if(it == this->dfp_mxa_map.end()){
        this->dfp_mxa_map.emplace(dfp_tag, std::move(ddi));
    }
    else{  
        it->second = ddi;
    }

    mx_retval_t ret(ddi.valid);
    return ret;
}

mx_retval_t DeviceManager::try_lock(int grp_id){
    mx_retval_t ret;
    if(stub_!=NULL){
        mxstream::MxService::Stub* local_stub = (mxstream::MxService::Stub*)stub_;
        LockData request;
        request.set_group_id(grp_id);
        Ping reply;
        grpc::ClientContext context;
        grpc::Status status_ = local_stub->try_lock(&context, request, &reply);
        if (!status_.ok()) {
            ret.error_flag = false;
            ret.error_msg = "daemon try_lock request failed; check the status of daemon";
            return ret;
        }
        ret.error_flag = reply.recv();
        if(!ret.error_flag) ret.error_msg = "Couldn't acquire lock on device "+std::to_string(grp_id);
        return ret;
    }
    if(server_mode_){
        ret.error_flag = true;
    } else {
        ret.error_flag = memx_status_no_error(memx_trylock(grp_id));
        if(!ret.error_flag) ret.error_msg = "Couldn't acquire lock on device "+std::to_string(grp_id);
    }
    return ret;
}

mx_retval_t DeviceManager::get_available_devices(){
    
    mx_retval_t ret;
    memx_status status = memx_operation_get_device_count(&all_devices_count);
    if (memx_status_error(status))
    {
        ret.error_flag = false;
        ret.error_msg = "Couldn't get device count";
        return ret;
    }

    // MX::Runtime::device_info di;
    for(int d = 0; d < all_devices_count ; d++){
        mx_retval_t lock_ret = this->try_lock(d);
        if(!lock_ret.error_flag){    
            std::cout<<"device locked - Trying next device \n";
        }
        else{
            MX::Runtime::device_info di;    
            uint8_t device_chip_count = 0;
            status = memx_get_total_chip_count(d, &device_chip_count);

            di.chip_count = device_chip_count;
            di.is_device_open = false;
            di.number_of_contexts_attached = 0;
            di.contexts_ids_attached = {};
            di.current_config = MEMX_MPU_GROUP_CONFIG_ONE_GROUP_FOUR_MPUS;
            
            auto device_it = this->available_mxa_device_map.find(d);
            
            if(device_it == this->available_mxa_device_map.end()){
                this->available_mxa_device_map.emplace(d , di);
                available_devices_id.push_back(d);
            }
            else{
                device_it->second = di;
            }
            this->device_unlock(d);
        }
    }
    ret.error_flag = true;
    return ret;
}

mx_retval_t DeviceManager::throw_chip_exception(int pdfp_chips, int pdevice_chips, int device_id){
    mx_retval_t ret;
    std::ostringstream oss;
    oss << "this dfp is made for " << pdfp_chips << " but only " << pdevice_chips << " are available on device "<<device_id;
    ret.error_flag = false;
    ret.error_msg = oss.str();
    return ret;
}

mx_retval_t DeviceManager::throw_mxa_gen_exception(int pdfp_num_chips){
    mx_retval_t ret;
    std::ostringstream oss;
    oss << "this dfp is made for CASCADE gen for " << pdfp_num_chips << "Cannot be configured at runtime \n";
    ret.error_flag = false;
    ret.error_msg = oss.str();
    return ret;
}

void DeviceManager::print_available_devices(){

    std::cout << "\n\tAvailable Devices: \n\n";
    if (available_devices_id.empty()) {
        std::cout << "None";
    } else {
            std::cout << "-----------------------------\n";
            std::cout << "| " << std::setw(10) << "Device ID" << " | " << std::setw(12) << "Chip Count" << " |\n";
            std::cout << "-----------------------------\n";

            for (size_t i = 0; i < available_devices_id.size(); i++) {
                int device_id = available_devices_id[i];
                std::cout << "| " << std::setw(10) << device_id
                          << " | " << std::setw(12) << available_mxa_device_map.at(device_id).chip_count
                          << " |\n";
            }

            std::cout << "-----------------------------\n\n";
    }
}

mx_retval_t DeviceManager::throw_device_not_available_exception(int pdevice_id){
    mx_retval_t ret;
    print_available_devices();
    ret.error_flag = false;
    ret.error_msg = "Device " + std::to_string(pdevice_id)+" is not available to use";
    return ret;
}


void DeviceManager::set_power_mode(int device_id, int num_chips){

  #ifdef __GNUC__
    // ignore the fact this variable is unused, to satisfy -Werror
    __attribute__((unused)) memx_status status;
  #else
    // else we're kind of stuck, lol
    memx_status status;
  #endif

    uint16_t c4_freq = 600;
    uint16_t c4_volt = 700;
    uint16_t c2_freq = 600;
    uint16_t c2_volt = 700;
   
  #ifdef __linux__
    // LINUX READ FILE

    if(std::filesystem::exists("/etc/memryx/power.conf")){

        // read each line
        std::ifstream fd("/etc/memryx/power.conf");
        for( std::string line; getline( fd, line ); ){
            if(line[0] == '#')
                continue;
            std::string varname = line.substr(0,6);
            if(varname == "FREQ4C"){
                std::string val = line.substr(7,3);
                c4_freq = (uint16_t) std::stoi(val);
            } else if(varname == "VOLT4C"){
                std::string val = line.substr(7,3);
                c4_volt = (uint16_t) std::stoi(val);
            } else if(varname == "FREQ2C"){
                std::string val = line.substr(7,3);
                c2_freq = (uint16_t) std::stoi(val);
            } else if(varname == "VOLT2C"){
                std::string val = line.substr(7,3);
                c2_volt = (uint16_t) std::stoi(val);
            }
        }

    }

    // else we use the defaults

  #else
    // Windows: just use defaults for now
  #endif

#ifdef __linux__
    // SET THE STUFF
    if(num_chips == 4){
        status = memx_set_feature(device_id, 0, OPCODE_SET_FREQUENCY, c4_freq);
        status = memx_set_feature(device_id, 1, OPCODE_SET_FREQUENCY, c4_freq);
        status = memx_set_feature(device_id, 2, OPCODE_SET_FREQUENCY, c4_freq);
        status = memx_set_feature(device_id, 3, OPCODE_SET_FREQUENCY, c4_freq);
        status = memx_set_feature(device_id, 0, OPCODE_SET_VOLTAGE, c4_volt);
    } else if(num_chips == 2){
        status = memx_set_feature(device_id, 0, OPCODE_SET_FREQUENCY, c2_freq);
        status = memx_set_feature(device_id, 1, OPCODE_SET_FREQUENCY, c2_freq);
        status = memx_set_feature(device_id, 0, OPCODE_SET_VOLTAGE, c2_volt);
    }
#else
    //Not supported for windows currently
#endif


}



mx_retval_t DeviceManager::configure_device(int device_id, int device_chip_count, int pdfp_num_chips, float pmxa_gen){

    memx_status status = MEMX_STATUS_OK;

    if(pmxa_gen == MEMX_DEVICE_CASCADE){
        mx_retval_t lock_ret = device_unlock(device_id);
        if(!lock_ret.error_flag) return lock_ret;
        mx_retval_t chip_ret = throw_mxa_gen_exception(device_chip_count);
        return chip_ret;
    }
    else{
        if(pdfp_num_chips>device_chip_count){
            mx_retval_t lock_ret = device_unlock(device_id);
            if(!lock_ret.error_flag) return lock_ret;
            mx_retval_t chip_ret = throw_chip_exception(pdfp_num_chips, device_chip_count, device_id);
            return chip_ret;
        }
        //Change the MPU config based on DFP if needed
        else if(pdfp_num_chips==8 && device_chip_count==8){
            status = memx_config_mpu_group(device_id, MEMX_MPU_GROUP_CONFIG_ONE_GROUP_EIGHT_MPUS);
            available_mxa_device_map.at(device_id).current_config = MEMX_MPU_GROUP_CONFIG_ONE_GROUP_EIGHT_MPUS;
        }
        else if(pdfp_num_chips==4){
            status = memx_config_mpu_group(device_id, MEMX_MPU_GROUP_CONFIG_ONE_GROUP_FOUR_MPUS);
            available_mxa_device_map.at(device_id).current_config = MEMX_MPU_GROUP_CONFIG_ONE_GROUP_FOUR_MPUS;
        }
        else if(pdfp_num_chips==2){
            status = memx_config_mpu_group(device_id, MEMX_MPU_GROUP_CONFIG_TWO_GROUP_TWO_MPUS);
            available_mxa_device_map.at(device_id).current_config = MEMX_MPU_GROUP_CONFIG_TWO_GROUP_TWO_MPUS;
        }
        else{
            mx_retval_t lock_ret = device_unlock(device_id);
            if(!lock_ret.error_flag) return lock_ret;
            mx_retval_t chip_ret = throw_chip_exception(pdfp_num_chips, device_chip_count, device_id);
            return chip_ret;
        }
    }

    mx_retval ret(true);
    if(memx_status_error(status)){
        ret.error_flag = false;
        ret.error_msg = "Device config error";
    }    
    return ret; 
}

mx_retval_t DeviceManager::connect_device(int dfp_tag, int device_id){

    mx_retval_t lock_ret = this->try_lock(device_id);
    // Lock MXA device
    if(!lock_ret.error_flag){
        return lock_ret;
    }
    
    // check chip count and see if dfp chip requires that much
    uint8_t device_chip_count = this->available_mxa_device_map.at(device_id).chip_count;
    int l_dfp_num_chips = this->dfp_mxa_map.at(dfp_tag).dfp_num_chips;
    float l_mxa_gen = this->dfp_mxa_map.at(dfp_tag).mxa_gen;
    bool l_use_mg_lb = this->dfp_mxa_map.at(dfp_tag).use_multigroup_lb;
    mx_retval_t config_ret(true);
    // if the dfp does not require a 8 chip MXA skip this device
    if(device_chip_count > 4 && l_dfp_num_chips <=4 ){
        mx_retval_t unlock_ret = device_unlock(device_id);
        if(!unlock_ret.error_flag) return unlock_ret;
        config_ret.error_flag= false;
        // continue;
    }
    else{
        //if device has the required number of chips configure the device and open necessary contexts    
        config_ret = configure_device(device_id, device_chip_count, l_dfp_num_chips, l_mxa_gen);
        if(l_dfp_num_chips == 4){
            set_power_mode(device_id, 4);
        } else if(l_dfp_num_chips == 2){
            if(l_use_mg_lb){
                set_power_mode(device_id, 4);
            } else {
                set_power_mode(device_id, 2);
            }
        }
        open_devices.push_back(device_id);            
        available_mxa_device_map.at(device_id).is_device_open = true;
    }
    return config_ret;
}

mx_retval_t DeviceManager::setup_mxa(int dfp_tag, std::vector<int>& pgroup_ids){

    
    required_devices = pgroup_ids.size();
    
    open_devices.clear();
    dfp_mxa_map.at(dfp_tag).context_ids_vector.clear();
    open_devices.reserve(required_devices);

    mx_retval_t config_ret(true);

    config_ret = this->get_available_devices();
    if(!config_ret.error_flag) return config_ret;

    for(int d = 0 ; d < required_devices ; d++){
        int device_id = pgroup_ids[d];
        if(available_mxa_device_map.find(device_id)==available_mxa_device_map.end()){
            return throw_device_not_available_exception(device_id);
        }
        available_mxa_device_map.at(device_id).contexts_ids_attached.clear();
        available_mxa_device_map.at(device_id).number_of_contexts_attached = 0;

        auto device_it = this->available_mxa_device_map.find(device_id);
        if (device_it != available_mxa_device_map.end() && !device_it->second.is_device_open) {

            config_ret = connect_device(dfp_tag, device_id);
            if(!config_ret.error_flag){
                return config_ret;
            }
        }
        else{
            return throw_device_not_available_exception(device_id);
        }
       
    }

    return config_ret;
}

mx_retval_t DeviceManager::attach_dfp_to_device(int dfp_tag){


    // While attaching dfp to an already configured device - check configuration and decide number of contexts required for tat dfp
    // this means that the dfps added later should have the same configuration as the initial dfp
    // if a new config dfp is added then setup and config has to be called again

    mx_retval_t attach_ret(true);

    for(int d = 0 ; d < required_devices ; d++){
        int device_id = open_devices[d];
        int number_of_contexts = 0;
        if(available_mxa_device_map.at(device_id).current_config == MEMX_MPU_GROUP_CONFIG_ONE_GROUP_FOUR_MPUS){
            number_of_contexts = 1;            
        }
        else{
            if(dfp_mxa_map.at(dfp_tag).use_multigroup_lb)
                number_of_contexts = 2;
            else
                number_of_contexts = 1;
        }

        //reserving two contexts per device as maximum two contexts are possible. (ignoring model swaping)
        int context_init = device_id*2;
        for(int i = 0; i < number_of_contexts; i++){
            // Context IDs are limited based on driver limitation (0 to 31) across all the devices
            // hence the context_id_tracker that keeps running count on all IDs
            // get the recent context_id to assign for a dfp
            
            if(context_init >= 32){
                attach_ret.error_flag = false;
                attach_ret.error_flag = "cannot open more than 32 contexts";
                return attach_ret;
            }

            memx_status status = memx_open(context_init, device_id, MEMX_DEVICE_CASCADE_PLUS);
            if (memx_status_error(status)){
                attach_ret.error_flag = false;
                attach_ret.error_flag = "Couldn't open a context with a device, please verify the MXA connection";
                return attach_ret;
            }
            else{
                dfp_mxa_map.at(dfp_tag).context_ids_vector.push_back(context_init);
                available_mxa_device_map.at(device_id).contexts_ids_attached.push_back(context_init);
                available_mxa_device_map.at(device_id).number_of_contexts_attached++;
                context_init++;
            }
        }

        int mpu_group_count = 0;
        memx_status status = memx_operation_get_mpu_group_count(device_id, &mpu_group_count);
        if (memx_status_error(status))
        {
            attach_ret.error_flag = false;
            attach_ret.error_flag = "Couldn't get the mpu group count";
            return attach_ret;
        }
    }
    return attach_ret;
}

mx_retval_t DeviceManager::download_dfp_to_device(int dfp_tag){

    mx_retval_t download_ret(true);
    // Since download of dfp has to happen a lot of times this function has been separated and can be called.
    // will download to all the contexts that has been assigned to that dfp and will enable the stream for that context
    int dfp_num_contexts = dfp_mxa_map.at(dfp_tag).context_ids_vector.size();
    for(int i = 0; i < dfp_num_contexts; i++){
        int ctx = dfp_mxa_map.at(dfp_tag).context_ids_vector[i];

        memx_status status;
        if(dfp_mxa_map.at(dfp_tag).is_bytes){
            status = memx_download_model(ctx,  (const char*) dfp_mxa_map.at(dfp_tag).dfp->src_dfp_bytes, 0 /*model_idx? */, MEMX_DOWNLOAD_TYPE_WTMEM_AND_MODEL_BUFFER);
        } else {
            status = memx_download_model(ctx,  dfp_mxa_map.at(dfp_tag).dfp->path().c_str(), 0 /*model_idx? */, MEMX_DOWNLOAD_TYPE_WTMEM_AND_MODEL);
        }
        if (memx_status_error(status))
        {
            std::ostringstream oss;
            oss<< "Download of DFP "<<  dfp_mxa_map.at(dfp_tag).dfp->path() <<" failed";
            download_ret.error_flag = false;
            download_ret.error_msg = oss.str();
            return download_ret;
        }

        // start stream
        status = memx_set_stream_enable(ctx, 0 /*wait time?*/);
        if (memx_status_error(status))
        {
            download_ret.error_flag = false;
            download_ret.error_msg = "Enable stream failed";
            return download_ret;
        }
    }
    return download_ret;
}

void DeviceManager::cleanup_dfp(int dfp_tag){
    auto it = this->dfp_mxa_map[dfp_tag];
    it.context_ids_vector.clear();

    if(it.dfp!=NULL){
        delete it.dfp;
        it.dfp = NULL;
    }
    else{
        it.dfp = NULL;
    }
    it.valid = false;    
}

void DeviceManager::cleanup__all_dfps(){
    for(auto& it  : this->dfp_mxa_map ){
        cleanup_dfp(it.first);
    }
    this->dfp_mxa_map.clear();
}

mx_retval_t DeviceManager::device_unlock(int device_id){
    mx_retval_t unlock_ret(true);
    if(stub_!=NULL){
        mxstream::MxService::Stub* local_stub = (mxstream::MxService::Stub*)stub_;
        grpc::ClientContext ctx;
        Ping reply;
        LockData lck_data;
        lck_data.set_group_id(device_id);
        grpc::Status status = local_stub->unlock(&ctx,lck_data,&reply);
        if(!status.ok()){
            unlock_ret.error_flag = false;
            unlock_ret.error_msg = "grpc unlock connection failed";
            return unlock_ret;
        }
        unlock_ret.error_flag = reply.recv();
        if(!unlock_ret.error_flag) unlock_ret.error_msg = "Daemon unlock failed on device: "+std::to_string(device_id);
        return unlock_ret;
    }
    if(server_mode_){
        unlock_ret.error_flag = true;
    } else {
        memx_status status = memx_unlock(device_id);
        unlock_ret.error_flag = memx_status_no_error(status);
        if(!unlock_ret.error_flag) unlock_ret.error_msg = "Unlock failed on device: "+std::to_string(device_id);
    }
    return unlock_ret;
}

mx_retval_t DeviceManager::close_device(int device_id){

    mx_retval_t close_ret(true);
    int num_contexts = available_mxa_device_map.at(device_id).number_of_contexts_attached;
    for(int ctx = 0; ctx < num_contexts ; ctx++){
        memx_status status;
        status = memx_close(available_mxa_device_map.at(device_id).contexts_ids_attached[ctx]);
        if (memx_status_error(status))
        {
            close_ret.error_flag = false;
            close_ret.error_msg = "MXA context close failed";
            return close_ret;
        }
    }
    available_mxa_device_map.at(device_id).number_of_contexts_attached = 0;
    available_mxa_device_map.at(device_id).is_device_open = false;
    available_mxa_device_map.at(device_id).contexts_ids_attached.clear();

    close_ret = device_unlock(device_id);
    return close_ret;
}

mx_retval_t DeviceManager::close_all_devices(){

    mx_retval_t close_ret(true);
    if(!open_devices.empty()){
        for (int i =0; i<static_cast<int>(open_devices.size()); i++){
            close_ret = close_device(open_devices[i]);
            if(!close_ret.error_flag) return close_ret;
        }
        open_devices.clear();
        available_devices_id.clear();
        available_mxa_device_map.clear();
    }
    return close_ret;
}

mx_retval_t DeviceManager::init_mx_models(int dfp_tag, std::vector<ModelBase *>* mxmodel_vector ){

    mx_retval_t model_ret(true);
    int num_models = dfp_mxa_map.at(dfp_tag).num_models;
    for (int i = 0; i < num_models; ++i)
    {

        vector<uint8_t> in_ports = dfp_mxa_map.at(dfp_tag).dfp_meta.model_inports[i];
        uint8_t format = dfp_mxa_map.at(dfp_tag).dfp->input_port(in_ports[0])->format;
        
        if(format == MX_FMT_RGB888){
            // MxModel<uint8_t> *im = new MxModel<uint8_t>(i, dfp_mxa_map.at(dfp_tag).dfp, &dfp_mxa_map.at(dfp_tag).context_ids_vector);
            // mxmodel_vector->push_back(im); 
            model_ret.error_flag = false;
            model_ret.error_msg = "int inputs are currently not supported";      
            return model_ret;         
        }
        else{
            MxModel<float> *fm = new MxModel<float>(i, dfp_mxa_map.at(dfp_tag).dfp, &dfp_mxa_map.at(dfp_tag).context_ids_vector);
            mxmodel_vector->push_back(fm);
        }
    }
    return model_ret;
}

// bool DeviceManager::dfp_tag_duplicate_check(int dfp_tag){
//     bool is_tag_duplicate;
//     auto it = this->dfp_mxa_map.find(dfp_tag);
//     if(it == this->dfp_mxa_map.end()){
//         is_tag_duplicate = false;
//     }
//     else{  
//         is_tag_duplicate =  true;
//     }

//     return is_tag_duplicate;

// }

int DeviceManager::get_dfp_num_chips(int dfp_tag){
    return dfp_mxa_map.at(dfp_tag).dfp_num_chips;
}

int DeviceManager::get_dfp_num_models(int dfp_tag){
    return dfp_mxa_map.at(dfp_tag).num_models;
}

bool DeviceManager::get_dfp_validity(int dfp_tag){
    return dfp_mxa_map.at(dfp_tag).valid;
}

int DeviceManager::get_num_outports(int dfp_tag){
    return dfp_mxa_map.at(dfp_tag).dfp_meta.num_outports;
}

// void DeviceManager::cleanup_all_setup_maps(){
    
//     DeviceManager::dfp_mxa_map.clear();
//     DeviceManager::available_mxa_device_map.clear();
// }

/*
// Additional get function disabled for now but might need later

float DeviceManager::get_dfp_mxa_gen(){
    return mxa_gen;
}

int DeviceManager::get_connected_devices_count(){
    return all_devices_count;

}

int DeviceManager::get_available_device_count(){
    return available_devices;
}


Dfp::DfpMeta DeviceManager::get_dfp_meta(){
    return this->dfp_meta;
}
*/
