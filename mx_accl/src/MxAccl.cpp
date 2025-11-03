// Copyright (c) 2025 MemryX
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <sstream>

#include "spdlog/spdlog.h"

#include <memx/accl/MxAccl.h>
#include <memx/accl/client.h>

using namespace MX::Runtime;
using namespace MX::Types;
using namespace MX::Utils;


MxAccl::~MxAccl()
{

    this->stop_all();

    // MxAcclBase destructor will delete all the rest
}

//---------------------------------------------------------------------------------------------
// ALL THE START/STOP/WAIT FUNCTIONS
//---------------------------------------------------------------------------------------------

void MxAccl::start(int model_id)
{
    if(model_id == -1) {
        this->start_all();
    }
    else {
        int dfp_id = 0; // TODO: temp solution
        this->start_model(dfp_id, model_id);
    }
}

void MxAccl::stop(int model_id)
{   
    if(model_id == -1) {
        this->stop_all();
    }
    else {
        int dfp_id = 0; // TODO: temp solution
        this->stop_model(dfp_id, model_id);
    }
}

void MxAccl::wait(int model_id)
{   
    if(model_id == -1) {
        this->wait_all();
    }
    else {
        int dfp_id = 0; // TODO: temp solution
        this->wait_model(dfp_id, model_id);
    }
}


// Start/Stop/Wait helpers
//------------------------
void MxAccl::start_model(int dfp_id, int model_id)
{
    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    if (it == runner_table.end()) {
        spdlog::error("[MxAccl] Error in start_model: dfp_id {} not found", dfp_id);
        lock.unlock();
        return;
    }
    DFPRunner* dfp_runner = it->second;
    if (dfp_runner == nullptr) {
        spdlog::error("[MxAccl] Error in start_model: dfp_runner is null");
        lock.unlock();
        return;
    }

    // access the MxModel for this DFPRunner
    MxModel* model = dfp_runner->models[model_id];
    if (model == nullptr) {
        spdlog::error("[MxAccl] Error in start_model: model is null");
        lock.unlock();
        return;
    }

    // don't need this lock anymore
    lock.unlock();

    // call start_model to this model
    model->model_start();
}

void MxAccl::stop_model(int dfp_id, int model_id)
{
    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    if (it == runner_table.end()) {
        spdlog::error("[MxAccl] Error in stop_model: dfp_id {} not found", dfp_id);
        lock.unlock();
        return;
    }
    DFPRunner* dfp_runner = it->second;
    if (dfp_runner == nullptr) {
        spdlog::error("[MxAccl] Error in stop_model: dfp_runner is null");
        lock.unlock();
        return;
    }

    // access the MxModel for this DFPRunner
    MxModel* model = dfp_runner->models[model_id];
    if (model == nullptr) {
        spdlog::error("[MxAccl] Error in stop_model: model is null");
        lock.unlock();
        return;
    }

    // don't need this lock anymore
    lock.unlock();

    // call stop_model to this model
    model->model_stop();
}

void MxAccl::wait_model(int dfp_id, int model_id)
{
    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    if (it == runner_table.end()) {
        spdlog::error("[MxAccl] Error in wait_model: dfp_id {} not found", dfp_id);
        lock.unlock();
        return;
    }
    DFPRunner* dfp_runner = it->second;
    if (dfp_runner == nullptr) {
        spdlog::error("[MxAccl] Error in wait_model: dfp_runner is null");
        lock.unlock();
        return;
    }

    // access the MxModel for this DFPRunner
    MxModel* model = dfp_runner->models[model_id];
    if (model == nullptr) {
        spdlog::error("[MxAccl] Error in wait_model: model is null");
        lock.unlock();
        return;
    }

    // don't need this lock anymore
    lock.unlock();

    // call wait_model to this model
    model->model_wait();
}

void MxAccl::start_dfp(int dfp_id)
{
    // for each model in the given dfp, call MxAccl::start_model()
    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    if (it == runner_table.end()) {
        spdlog::error("[MxAccl] Error in start_dfp: dfp_id {} not found", dfp_id);
        lock.unlock();
        return;
    }
    DFPRunner* dfp_runner = it->second;
    if (dfp_runner == nullptr) {
        spdlog::error("[MxAccl] Error in start_dfp: dfp_runner is null");
        lock.unlock();
        return;
    }
    lock.unlock();
    int num_models = dfp_runner->num_models;
    for(int i = 0; i < num_models; i++) {
        this->start_model(dfp_id, i);
    }
}

void MxAccl::stop_dfp(int dfp_id)
{
    // for each model in the given dfp, call MxAccl::stop_model()
    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    if (it == runner_table.end()) {
        spdlog::error("[MxAccl] Error in stop_dfp: dfp_id {} not found", dfp_id);
        lock.unlock();
        return;
    }
    DFPRunner* dfp_runner = it->second;
    if (dfp_runner == nullptr) {
        spdlog::error("[MxAccl] Error in stop_dfp: dfp_runner is null");
        lock.unlock();
        return;
    }
    lock.unlock();
    int num_models = dfp_runner->num_models;
    for(int i = 0; i < num_models; i++) {
        this->stop_model(dfp_id, i);
    }
    
    // Close the DFP
    lock.lock();
    if(dfp_runner->is_local()) {
        dfp_runner->close_local();
    }
    else {
        dfp_runner->close_shared();
    }
    runner_table.erase(dfp_id);
    // remove any items in device_to_dfp_id_map whose value is dfp_id
    for(auto it = device_to_dfp_id_map.begin(); it != device_to_dfp_id_map.end();) {
        if(it->second == dfp_id) {
            it = device_to_dfp_id_map.erase(it);
        }
        else {
            ++it;
        }
    }
    delete dfp_runner;
    dfp_runner = nullptr;

    lock.unlock();
}

void MxAccl::wait_dfp(int dfp_id)
{
    // for each model in the given dfp, call MxAccl::wait_model()
    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    if (it == runner_table.end()) {
        spdlog::error("[MxAccl] Error in wait_dfp: dfp_id {} not found", dfp_id);
        lock.unlock();
        return;
    }
    DFPRunner* dfp_runner = it->second;
    if (dfp_runner == nullptr) {
        spdlog::error("[MxAccl] Error in wait_dfp: dfp_runner is null");
        lock.unlock();
        return;
    }
    lock.unlock();
    int num_models = dfp_runner->num_models;
    for(int i = 0; i < num_models; i++) {
        this->wait_model(dfp_id, i);
    }
}

void MxAccl::start_all()
{
    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    int num_dfps = runner_table.size();
    if(num_dfps == 0) {
        spdlog::error("[MxAccl] Error in start_all: no dfps found");
        lock.unlock();
        return;
    }
    lock.unlock();
    for(auto &id : runner_table) {
        this->start_dfp(id.first);
    }
}

void MxAccl::stop_all()
{
    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    int num_dfps = runner_table.size();
    if(num_dfps == 0) {
        spdlog::debug("[MxAccl] stop_all called but no open dfps found");
        lock.unlock();
        return;
    }
    lock.unlock();

    // go through runner_table and stop each dfp, but
    // note that stop_dfp will also remove it from the table -- need
    // to be able to handle this in the loop(s)
    //
    // resiliant but wasteful way to do this is to copy the keys into a vector
    // and iterate over that vector
    std::vector<int> dfp_ids;
    {
        std::shared_lock<std::shared_mutex> lock(runner_mutex);
        for(const auto &id : runner_table) {
            dfp_ids.push_back(id.first);
        }
        lock.unlock();
    }
    for(const auto &dfp_id : dfp_ids) {
        this->stop_dfp(dfp_id);
    }
}

void MxAccl::wait_all()
{
    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    int num_dfps = runner_table.size();
    if(num_dfps == 0) {
        spdlog::error("[MxAccl] Error in wait_all: no dfps found");
        lock.unlock();
        return;
    }
    lock.unlock();
    for(auto &id : runner_table) {
        this->wait_dfp(id.first);
    }
}

//---------------------------------------------------------------------------------------------
// CONNECT_STREAM AND FRIENDS
//---------------------------------------------------------------------------------------------

void MxAccl::connect_stream(float_callback_t in_cb, float_callback_t out_cb, int stream_id, int model_id)
{
    int dfp_id = 0; // TODO: temp solution

    // get the DFPRunner for this dfp_id (if it exists in the map)
    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    if (it == runner_table.end()) {
        spdlog::error("[MxAccl] Error in connect_stream: dfp_id {} not found", dfp_id);
        lock.unlock();
        return;
    }
    DFPRunner* dfp_runner = it->second;
    if (dfp_runner == nullptr) {
        spdlog::error("[MxAccl] Error in connect_stream: dfp_runner is null");
        lock.unlock();
        return;
    }

    // access the MxModel for this DFPRunner
    MxModel* model = dfp_runner->models[model_id];
    if (model == nullptr) {
        spdlog::error("[MxAccl] Error in connect_stream: model is null");
        lock.unlock();
        return;
    }

    // don't need this lock anymore
    lock.unlock();

    // call connect_stream to this model with the given callbacks
    model->connect_stream(in_cb, out_cb, stream_id);
}

void MxAccl::set_num_workers(int input_num_workers, int output_num_workers, int model_id)
{
    int dfp_id = 0; // TODO: temp solution

    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    if(it == runner_table.end()) {
        spdlog::error("[MxAccl] Error in set_num_workers: dfp_id {} not found", dfp_id);
        lock.unlock();
        throw std::runtime_error("MxAccl: Error in set_num_workers: dfp_id not found");
    }

    lock.unlock();

    DFPRunner* dfp_runner = it->second;
    dfp_runner->models[model_id]->set_num_workers(input_num_workers, output_num_workers);
}

int MxAccl::get_num_streams(int model_id)
{
    int dfp_id = 0; // TODO: temp solution

    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    if(it == runner_table.end()) {
        spdlog::error("[MxAccl] Error in get_num_streams: dfp_id {} not found", dfp_id);
        lock.unlock();
        throw std::runtime_error("MxAccl: Error in get_num_streams: dfp_id not found");
    }

    lock.unlock();

    DFPRunner* dfp_runner = it->second;
    return dfp_runner->models[model_id]->get_num_streams();
}

