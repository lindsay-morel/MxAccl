// Copyright (c) 2025 MemryX
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <sstream>

#include "spdlog/spdlog.h"

#include <memx/accl/MxAcclBase.h>
#include <memx/accl/client.h>
#include <memx/accl/utils/cpu_opts.h>

using namespace MX::Runtime;
using namespace MX::Types;
using namespace MX::Utils;
using namespace MX::RPC;
using namespace std;

// basic constructor -- for calling connect_dfp later
MxAcclBase::MxAcclBase(std::string server_addr, unsigned int server_port_base, bool ignore_server)
{
    ignore_server_ = ignore_server;
    server_addr_ = server_addr;
    server_port_base_ = server_port_base;

    // set my thread affinity to big cores, requiring at least 4
    set_self_affinity_to_big_cores(4);

    device_manager = new MX::Runtime::DeviceManager();
}

// AIO constructor using file path
MxAcclBase::MxAcclBase(const std::filesystem::path& dfp_path, std::vector<int> device_ids_to_use,
                       std::array<bool, 2> use_model_shape, bool local_mode,
                       SchedulerOptions sched_options, ClientOptions client_options,
                       std::string server_addr, unsigned int server_port_base, bool ignore_server)
    : MxAcclBase(server_addr, server_port_base, ignore_server)
{
    // connect the dfp
    int dfp_id = connect_dfp(dfp_path, device_ids_to_use, use_model_shape, local_mode, sched_options, client_options);
    if(dfp_id < 0) {
        std::string err_msg = "[MxAcclBase] Error in MxAcclBase constructor: connect_dfp failed.";
        spdlog::error(err_msg);
        throw std::runtime_error(err_msg);
    }

    if(local_mode){
        // init pressure history
        pressure_thread_running.store(true, std::memory_order_relaxed);
        pressure_history.resize(device_manager->all_devices_count);
        pressure_avgs.resize(device_manager->all_devices_count, 0.0f);
        pressure_thread = new std::thread(&MxAcclBase::pressure_thread_func, this, device_ids_to_use);
        pressure_thread->detach();
    } else {
        pressure_thread = nullptr;
        pressure_thread_running.store(false, std::memory_order_relaxed);
    }
}


// AIO constructor using bytes
MxAcclBase::MxAcclBase(uint8_t* dfp_bytes, size_t dfp_byte_size, std::vector<int> device_ids_to_use,
                       std::array<bool, 2> use_model_shape, bool local_mode,
                       SchedulerOptions sched_options, ClientOptions client_options,
                       std::string server_addr, unsigned int server_port_base, bool ignore_server)
    : MxAcclBase(server_addr, server_port_base, ignore_server)
{
    // connect the dfp
    int dfp_id = connect_dfp(dfp_bytes, dfp_byte_size, device_ids_to_use, use_model_shape, local_mode, sched_options, client_options);
    if(dfp_id < 0) {
        std::string err_msg = "[MxAcclBase] Error in MxAcclBase constructor: connect_dfp failed.";
        spdlog::error(err_msg);
        throw std::runtime_error(err_msg);
    }
    
    if(local_mode){
        // init pressure history
        pressure_thread_running.store(true, std::memory_order_relaxed);
        pressure_history.resize(device_manager->all_devices_count);
        pressure_avgs.resize(device_manager->all_devices_count, 0.0f);
        pressure_thread = new std::thread(&MxAcclBase::pressure_thread_func, this, device_ids_to_use);
        pressure_thread->detach();
    } else {
        pressure_thread = nullptr;
        pressure_thread_running.store(false, std::memory_order_relaxed);
    }
}

MxAcclBase::~MxAcclBase()
{
    // stop pressure thread if local mode
    if(pressure_thread != nullptr) {
        pressure_thread_running.store(false, std::memory_order_relaxed);
        delete pressure_thread;
        pressure_thread = nullptr;
    }
    // close all dfp_runners
    for(auto it = runner_table.begin(); it != runner_table.end(); it++) {
        DFPRunner* dfp_runner = it->second;
        if(dfp_runner->is_local()) {
            dfp_runner->close_local();
        }
        else {
            dfp_runner->close_shared();
        }
        delete dfp_runner;
    }
    runner_table.clear();
    device_to_dfp_id_map.clear();
    delete device_manager;
}

bool MxAcclBase::is_ready()
{
    // check if we have any valid DFPRunners, or if ignore_server_ is set to true then we check if the device manager has > 0 devices
    if(ignore_server_) {
        return device_manager->all_devices_count > 0;
    }
    else {
        return !runner_table.empty();
    }
}

//==============================================================================================
// LOCAL MODE PRESSURE MONITOR
//==============================================================================================

void MxAcclBase::pressure_thread_func(std::vector<int> device_ids){
    // this thread periodically polls each device in device_ids
    // and updates an average pressure value for each device
    // the get_pressure function just returns the average
    while(pressure_thread_running.load(std::memory_order_relaxed)) {

        for(auto device_id : device_ids) {
            if(device_id < 0 || device_id >= device_manager->all_devices_count) {
                continue;
            }
            float p = device_manager->get_pressure(device_id);
            if(p < 0.0f || p > 100.0f) {
                continue;
            }
            auto& history = pressure_history[device_id];
            history.push_back(p);
            if(history.size() > pressure_history_len) {
                history.pop_front();
            }

            // average the history into the pressure_avgs vect
            float sum = 0.0f;
            #pragma omp simd reduction(+:sum)
            for(unsigned int i = 0; i < history.size(); i++) {
                sum += history[i];
            }
            pressure_avgs[device_id] = sum / static_cast<float>(history.size());
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(pressure_poll_interval_ms));
    }
}


//==============================================================================================
// CONNECT DFP FUNCTIONS
//==============================================================================================

// multi-device from bytes
int MxAcclBase::connect_dfp(uint8_t* dfp_bytes, size_t dfp_byte_size, 
                            std::vector<int> device_ids_to_use,
                            std::array<bool, 2> use_model_shape,
                            bool local_mode,
                            SchedulerOptions sched_options, ClientOptions client_options)
{

    int dfp_id = -1;
    if(dfp_bytes == nullptr) {
        spdlog::error("[MxAcclBase] Error in connect_dfp: dfp_bytes is null");
        return -1;
    }

    // cannot have local_mode=false and ignore_server_=true at the same time
    if(ignore_server_ && !local_mode) {
        spdlog::error("[MxAcclBase] Error in connect_dfp: cannot have ignore_server with local_mode=false");
        return -1;
    }

    // generate a new dfp_id
    dfp_id = dfp_id_tracker.get_new();

    // parse the dfp_bytes into a Dfp::DfpObject
    Dfp::DfpObject* dfp = new Dfp::DfpObject(dfp_bytes, dfp_byte_size);

    // create the runner
    DFPRunner* dfp_runner = new DFPRunner(dfp_id, dfp, server_addr_, server_port_base_, local_mode, device_ids_to_use,
                                          use_model_shape, sched_options, client_options, device_manager, ignore_server_);

    // if local mode, set dfp_runner->init_local() and record lock successes/fails
    if(local_mode) {
        if(dfp_runner->init_local() == false) {
            spdlog::error("[MxAcclBase] Error in connect_dfp: dfp_runner->init_local() failed");
            delete dfp_runner;
            dfp_id_tracker.retire(dfp_id);
            return -1;
        }
        else {
            // add the runner to the table and local devices in use list
            std::unique_lock<std::shared_mutex> lock(runner_mutex);
            runner_table[dfp_id] = dfp_runner;
            for(auto device_id : device_ids_to_use) {
                device_manager->local_device_in_use[device_id] = true;
                device_to_dfp_id_map[device_id] = dfp_id;
            }
            lock.unlock();
        }
    }
    else {
        if(dfp_runner->init_shared() == false) {
            spdlog::error("[MxAcclBase] Error in connect_dfp: dfp_runner->init_shared() failed");
            delete dfp_runner;
            dfp_id_tracker.retire(dfp_id);
            return -1;
        }
        else {
            // add the runner to the table
            std::unique_lock<std::shared_mutex> lock(runner_mutex);
            runner_table[dfp_id] = dfp_runner;
            for(auto device_id : device_ids_to_use) {
                device_to_dfp_id_map[device_id] = dfp_id;
            }
            lock.unlock();
        }
    }

    // return the dfp_id
    return dfp_id;
}

// multi-device from file
int MxAcclBase::connect_dfp(const std::filesystem::path dfp_path, std::vector<int> device_ids_to_use,
                            std::array<bool, 2> use_model_shape,
                            bool local_mode,
                            SchedulerOptions sched_options, ClientOptions client_options)
{

    int dfp_id = -1;
    if(dfp_path.empty()) {
        spdlog::error("[MxAcclBase] Error in connect_dfp: dfp_path is empty");
        return -1;
    }

    // cannot have local_mode=false and ignore_server_=true at the same time
    if(ignore_server_ && !local_mode) {
        spdlog::error("[MxAcclBase] Error in connect_dfp: cannot have ignore_server with local_mode=false");
        return -1;
    }

    // generate a new dfp_id
    dfp_id = dfp_id_tracker.get_new();

    // parse the dfp_bytes into a Dfp::DfpObject
    Dfp::DfpObject* dfp = new Dfp::DfpObject(dfp_path.string());

    // create the runner
    DFPRunner* dfp_runner = new DFPRunner(dfp_id, dfp, server_addr_, server_port_base_, local_mode, device_ids_to_use,
                                          use_model_shape, sched_options, client_options, device_manager, ignore_server_);

    // if local mode, set dfp_runner->init_local() and record lock successes/fails
    if(local_mode) {
        if(dfp_runner->init_local() == false) {
            spdlog::error("[MxAcclBase] Error in connect_dfp: dfp_runner->init_local() failed");
            delete dfp_runner;
            dfp_id_tracker.retire(dfp_id);
            return -1;
        }
        else {
            // add the runner to the table and local devices in use list
            std::unique_lock<std::shared_mutex> lock(runner_mutex);
            runner_table[dfp_id] = dfp_runner;
            for(auto device_id : device_ids_to_use) {
                device_manager->local_device_in_use[device_id] = true;
                device_to_dfp_id_map[device_id] = dfp_id;
            }
            lock.unlock();
        }
    }
    else {
        if(dfp_runner->init_shared() == false) {
            spdlog::error("[MxAcclBase] Error in connect_dfp: dfp_runner->init_shared() failed");
            delete dfp_runner;
            dfp_id_tracker.retire(dfp_id);
            return -1;
        }
        else {
            // add the runner to the table
            std::unique_lock<std::shared_mutex> lock(runner_mutex);
            runner_table[dfp_id] = dfp_runner;
            for(auto device_id : device_ids_to_use) {
                device_to_dfp_id_map[device_id] = dfp_id;
            }
            lock.unlock();
        }
    }

    // return the dfp_id
    return dfp_id;

}


//==============================================================================================
// REMOVE DFP
//==============================================================================================

bool MxAcclBase::remove_dfp()
{
    int dfp_id = 0; // TODO: temp solution
    std::unique_lock<std::shared_mutex> lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    if(it == runner_table.end()) {
        spdlog::error("[MxAcclBase] Error in remove_dfp: dfp_id {} not found", dfp_id);
        lock.unlock();
        return false;
    }

    // get the dfp_runner
    DFPRunner* dfp_runner = it->second;

    // used to track the device ids that we need to search for
    // potential new owners in the device_to_dfp_id_map
    std::deque<int> device_ids_to_redo;

    // close the dfp_runner
    if(dfp_runner->is_local()) {
        if(dfp_runner->close_local() == false) {
            spdlog::error("[MxAcclBase] Error in remove_dfp: dfp_runner->close_local() failed");
            lock.unlock();
            return false;
        }
        else {
            // remove the local devices from the in use list
            for(auto device_id : dfp_runner->device_ids_to_use_) {
                device_manager->local_device_in_use[device_id] = false;
                device_to_dfp_id_map.erase(device_id);
                device_ids_to_redo.push_back(device_id);
            }
        }
    }
    else {
        if(dfp_runner->close_shared() == false) {
            spdlog::error("[MxAcclBase] Error in remove_dfp: dfp_runner->close_shared() failed");
            lock.unlock();
            return false;
        }

        // remove the device ids from the device_to_dfp_id_map
        for(auto device_id : dfp_runner->device_ids_to_use_) {
            auto it_device = device_to_dfp_id_map.find(device_id);
            if(it_device != device_to_dfp_id_map.end()) {
                device_to_dfp_id_map.erase(it_device);
                device_ids_to_redo.push_back(device_id);
            }
            else {
                spdlog::warn("[MxAcclBase] Warning in remove_dfp: device_id {} not found in device_to_dfp_id_map", device_id);
            }
        }
    }

    // delete the dfp_runner
    delete dfp_runner;

    // remove the dfp_runner from the table
    runner_table.erase(it);

    // retire the dfp_id from the tracker
    dfp_id_tracker.retire(dfp_id);


    // for each device_id in device_ids_to_redo, go through the runner_table
    // and see if any other dfp_runner is using that device_id.
    //
    // if so, update the device_to_dfp_id_map to point to that dfp_id
    for(auto device_id : device_ids_to_redo) {
        for(auto &runner_pair : runner_table) {
            DFPRunner* runner = runner_pair.second;
            if(std::find(runner->device_ids_to_use_.begin(), runner->device_ids_to_use_.end(), device_id) !=
                    runner->device_ids_to_use_.end()) {
                // found a dfp_runner that is using this device_id
                device_to_dfp_id_map[device_id] = runner_pair.first; // update the map to point to this dfp_id
                break;
            }
        }
    }

    lock.unlock();
    return true;
}


//==============================================================================================
// MISC "GET" FUNCTIONS
//==============================================================================================

int MxAcclBase::get_num_models()
{
    int dfp_id = 0; // TODO: temp solution

    std::shared_lock lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    if(it == runner_table.end()) {
        spdlog::error("[MxAcclBase] Error in get_num_models: dfp_id {} not found", dfp_id);
        lock.unlock();
        return -1;
    }
    DFPRunner* dfp_runner = it->second;
    lock.unlock();
    return dfp_runner->num_models;
}

int MxAcclBase::get_dfp_num_chips()
{
    int dfp_id = 0; // TODO: temp solution

    std::shared_lock lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    if(it == runner_table.end()) {
        spdlog::error("[MxAcclBase] Error in get_dfp_num_chips: dfp_id {} not found", dfp_id);
        lock.unlock();
        return -1;
    }
    DFPRunner* dfp_runner = it->second;
    lock.unlock();
    return dfp_runner->dfp_->get_dfp_meta()->num_chips;
}

MxModelInfo MxAcclBase::get_model_info(int model_id) const
{
    int dfp_id = 0; // TODO: temp solution

    std::shared_lock lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    if(it == runner_table.end()) {
        spdlog::error("[MxAcclBase] Error in get_model_info: dfp_id {} not found", dfp_id);
        lock.unlock();
        throw std::runtime_error("MxAcclBase: Error in get_model_info: dfp_id not found");
    }
    DFPRunner* dfp_runner = it->second;
    lock.unlock();
    return dfp_runner->models[model_id]->return_model_info();
}

MxModelInfo MxAcclBase::get_pre_model_info(int model_id) const
{
    int dfp_id = 0; // TODO: temp solution

    std::shared_lock lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    if(it == runner_table.end()) {
        spdlog::error("[MxAcclBase] Error in get_pre_model_info: dfp_id {} not found", dfp_id);
        lock.unlock();
        throw std::runtime_error("MxAcclBase: Error in get_pre_model_info: dfp_id not found");
    }
    DFPRunner* dfp_runner = it->second;
    lock.unlock();
    return dfp_runner->models[model_id]->return_pre_model_info();
}

MxModelInfo MxAcclBase::get_post_model_info(int model_id) const
{
    int dfp_id = 0; // TODO: temp solution

    std::shared_lock lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    if(it == runner_table.end()) {
        spdlog::error("[MxAcclBase] Error in get_post_model_info: dfp_id {} not found", dfp_id);
        lock.unlock();
        throw std::runtime_error("MxAcclBase: Error in get_post_model_info: dfp_id not found");
    }
    DFPRunner* dfp_runner = it->second;
    lock.unlock();
    return dfp_runner->models[model_id]->return_post_model_info();
}

//==============================================================================================
// MISC "SET" FUNCTIONS
//==============================================================================================

void MxAcclBase::connect_post_model(std::filesystem::path post_model_path, int model_id,
                                    const std::vector<size_t> &post_size_list)
{
    int dfp_id = 0; // TODO: temp solution

    std::shared_lock lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    if(it == runner_table.end()) {
        spdlog::error("[MxAcclBase] Error in connect_post_model: dfp_id {} not found", dfp_id);
        lock.unlock();
        throw std::runtime_error("MxAcclBase: Error in connect_post_model: dfp_id not found");
    }
    DFPRunner* dfp_runner = it->second;
    dfp_runner->models[model_id]->model_set_post(post_model_path, post_size_list);
    lock.unlock();
}

void MxAcclBase::connect_pre_model(std::filesystem::path pre_model_path, int model_id)
{
    int dfp_id = 0; // TODO: temp solution

    std::shared_lock lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    if(it == runner_table.end()) {
        spdlog::error("[MxAcclBase] Error in connect_pre_model: dfp_id {} not found", dfp_id);
        lock.unlock();
        throw std::runtime_error("MxAcclBase: Error in connect_pre_model: dfp_id not found");
    }
    DFPRunner* dfp_runner = it->second;
    dfp_runner->models[model_id]->model_set_pre(pre_model_path);
    lock.unlock();
}

void MxAcclBase::set_parallel_fmap_convert(int num_threads, int model_id)
{
    int dfp_id = 0; // TODO: temp solution

    std::shared_lock lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    if(it == runner_table.end()) {
        spdlog::error("[MxAcclBase] Error in set_parallel_fmap_convert: dfp_id {} not found", dfp_id);
        lock.unlock();
        throw std::runtime_error("MxAcclBase: Error in set_parallel_fmap_convert: dfp_id not found");
    }
    DFPRunner* dfp_runner = it->second;
    dfp_runner->models[model_id]->set_parallel_fmap_convert(num_threads);
    lock.unlock();
}

//==============================================================================================
// POWER, TEMP, AND PRESSURE FUNCTIONS
//==============================================================================================

bool MxAcclBase::can_get_power_consumption(int device_id)
{
    if(device_manager == nullptr) {
        spdlog::error("[MxAcclBase] Error in can_get_power_consumption: device_manager is null");
        return false;
    }

    if(device_id < 0 || device_id >= device_manager->all_devices_count) {
        spdlog::error("[MxAcclBase] Error in can_get_power_consumption: device_id {} is out of range", device_id);
        return false;
    }

    // this function doesn't actually differ between local and shared mode
    return device_manager->device_infos[device_id].can_get_power_data;
}

float MxAcclBase::get_power(int device_id)
{
    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    // get the DFPRunner for this device_id
    auto it = device_to_dfp_id_map.find(device_id);
    if(UNLIKELY(it == device_to_dfp_id_map.end())) {
        spdlog::error("[MxAcclBase] Error in get_power: device_id {} not found in device_to_dfp_id_map", device_id);
        lock.unlock();
        return -1;
    }
    int dfp_id = it->second;
    // get the DFPRunner
    auto dfp_it = runner_table.find(dfp_id);
    if(UNLIKELY(dfp_it == runner_table.end())) {
        spdlog::error("[MxAcclBase] Error in get_power: dfp_id {} not found in runner_table", dfp_id);
        lock.unlock();
        return -1;
    }

    bool is_local = dfp_it->second->is_local();

    if(!is_local) {
        Client* client = dfp_it->second->get_first_client();
        lock.unlock();

        // call client->get_avg_max_temp(device_id)
        if(UNLIKELY(client == nullptr)) {
            spdlog::error("[MxAcclBase] Error in get_power: client is null");
            return -1;
        }

        return client->get_avg_power(device_id);
    }
    else {
        lock.unlock();
        if(device_id < 0 || device_id >= device_manager->all_devices_count) {
            spdlog::error("[MxAcclBase] Error in get_power: device_id {} is out of range", device_id);
            return -1;
        }

        return device_manager->get_power(device_id);
    }
}

float MxAcclBase::get_max_temperature(int device_id)
{
    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    // get the DFPRunner for this device_id
    auto it = device_to_dfp_id_map.find(device_id);
    if(UNLIKELY(it == device_to_dfp_id_map.end())) {
        spdlog::error("[MxAcclBase] Error in get_max_temperature: device_id {} not found in device_to_dfp_id_map", device_id);
        lock.unlock();
        return -1;
    }
    int dfp_id = it->second;
    // get the DFPRunner
    auto dfp_it = runner_table.find(dfp_id);
    if(UNLIKELY(dfp_it == runner_table.end())) {
        spdlog::error("[MxAcclBase] Error in get_max_temperature: dfp_id {} not found in runner_table", dfp_id);
        lock.unlock();
        return -1;
    }

    bool is_local = dfp_it->second->is_local();

    if(!is_local) {
        Client* client = dfp_it->second->get_first_client();
        lock.unlock();

        // call client->get_avg_max_temp(device_id)
        if(UNLIKELY(client == nullptr)) {
            spdlog::error("[MxAcclBase] Error in get_max_temperature: client is null");
            return -1;
        }

        return client->get_inst_max_temp(device_id);
    }
    else {
        lock.unlock();
        if(device_id < 0 || device_id >= device_manager->all_devices_count) {
            spdlog::error("[MxAcclBase] Error in get_max_temperature: device_id {} is out of range", device_id);
            return -1;
        }

        return device_manager->get_max_temperature(device_id);
    }
}

std::vector<float> MxAcclBase::get_chip_temperatures(int device_id)
{
    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    // get the DFPRunner for this device_id
    auto it = device_to_dfp_id_map.find(device_id);
    if(UNLIKELY(it == device_to_dfp_id_map.end())) {
        spdlog::error("[MxAcclBase] Error in get_chip_temperatures: device_id {} not found in device_to_dfp_id_map", device_id);
        lock.unlock();
        return std::vector<float>();
    }
    int dfp_id = it->second;
    // get the DFPRunner
    auto dfp_it = runner_table.find(dfp_id);
    if(UNLIKELY(dfp_it == runner_table.end())) {
        spdlog::error("[MxAcclBase] Error in get_chip_temperatures: dfp_id {} not found in runner_table", dfp_id);
        lock.unlock();
        return std::vector<float>();
    }

    bool is_local = dfp_it->second->is_local();

    if(!is_local) {
        Client* client = dfp_it->second->get_first_client();
        lock.unlock();

        // call client->get_avg_max_temp(device_id)
        if(UNLIKELY(client == nullptr)) {
            spdlog::error("[MxAcclBase] Error in get_chip_temperatures: client is null");
            return std::vector<float>();
        }

        return client->get_avg_temp_per_chip(device_id);
    }
    else {
        lock.unlock();
        if(device_id < 0 || device_id >= device_manager->all_devices_count) {
            spdlog::error("[MxAcclBase] Error in get_chip_temperatures: device_id {} is out of range", device_id);
            return std::vector<float>();
        }

        return device_manager->get_chip_temperatures(device_id);
    }
}

Pressure MxAcclBase::get_pressure(int device_id)
{
    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    // get the DFPRunner for this device_id
    auto it = device_to_dfp_id_map.find(device_id);
    if(UNLIKELY(it == device_to_dfp_id_map.end())) {
        spdlog::error("[MxAcclBase] Error in get_pressure: device_id {} not found in device_to_dfp_id_map", device_id);
        lock.unlock();
        return Pressure(Pressure::Level::FULL);
    }
    int dfp_id = it->second;
    // get the DFPRunner
    auto dfp_it = runner_table.find(dfp_id);
    if(UNLIKELY(dfp_it == runner_table.end())) {
        spdlog::error("[MxAcclBase] Error in get_pressure: dfp_id {} not found in runner_table", dfp_id);
        lock.unlock();
        return Pressure(Pressure::Level::FULL);
    }

    bool is_local = dfp_it->second->is_local();

    if(!is_local) {
        Client* client = dfp_it->second->get_first_client();
        lock.unlock();

        // call client->get_pressure(device_id)
        if(UNLIKELY(client == nullptr)) {
            spdlog::error("[MxAcclBase] Error in get_pressure: client is null");
        }

        float p = client->get_pressure(device_id);
        if(p < MEMX_PRESSURE_LOW_THRESH) {
            return Pressure(Pressure::Level::LOW);
        }
        else if(p < MEMX_PRESSURE_MEDIUM_THRESH) {
            return Pressure(Pressure::Level::MEDIUM);
        }
        else if(p < MEMX_PRESSURE_HIGH_THRESH) {
            return Pressure(Pressure::Level::HIGH);
        }
        else {
            return Pressure(Pressure::Level::FULL);
        }
    }
    else {
        lock.unlock();
        if(device_id < 0 || ((unsigned int)device_id) >= pressure_avgs.size()) {
            spdlog::error("[MxAcclBase] Error in get_pressure: device_id {} is out of range", device_id);
            return -1;
        }

        float p = pressure_avgs[device_id];
        if(p < MEMX_PRESSURE_LOW_THRESH) {
            return Pressure(Pressure::Level::LOW);
        }
        else if(p < MEMX_PRESSURE_MEDIUM_THRESH) {
            return Pressure(Pressure::Level::MEDIUM);
        }
        else if(p < MEMX_PRESSURE_HIGH_THRESH) {
            return Pressure(Pressure::Level::HIGH);
        }
        else {
            return Pressure(Pressure::Level::FULL);
        }
    }
}


//==============================================================================================
// DEVICE CONTROL FUNCTIONS
//==============================================================================================

bool MxAcclBase::set_operating_frequency(int device_id, MxFrequencyOption freq_option)
{
    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    // get the DFPRunner for this device_id
    auto it = device_to_dfp_id_map.find(device_id);
    if(UNLIKELY(it == device_to_dfp_id_map.end())) {
        spdlog::error("[MxAcclBase] Error in set_operating_frequency: device_id {} not found in device_to_dfp_id_map", device_id);
        lock.unlock();
        return false;
    }
    int dfp_id = it->second;
    // get the DFPRunner
    auto dfp_it = runner_table.find(dfp_id);
    if(UNLIKELY(dfp_it == runner_table.end())) {
        spdlog::error("[MxAcclBase] Error in set_operating_frequency: dfp_id {} not found in runner_table", dfp_id);
        lock.unlock();
        return false;
    }

    bool is_local = dfp_it->second->is_local();

    if(!is_local) {
        Client* client = dfp_it->second->get_first_client();
        lock.unlock();

        // call client->get_avg_max_temp(device_id)
        if(UNLIKELY(client == nullptr)) {
            spdlog::error("[MxAcclBase] Error in set_operating_frequency: client is null");
            return false;
        }

        return client->set_power_mode(device_id, (uint16_t) freq_option);
    }
    else {
        lock.unlock();
        if(device_id < 0 || device_id >= device_manager->all_devices_count) {
            spdlog::error("[MxAcclBase] Error in set_operating_frequency: device_id {} is out of range", device_id);
            return false;
        }

        // get the device chip count from device_manager
        int device_chip_count = device_manager->device_infos[device_id].chip_count;

        return device_manager->set_power_mode(device_id, device_chip_count, freq_option);
    }

}
