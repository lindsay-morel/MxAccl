// Copyright (c) 2025 MemryX
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <sstream>

#include "spdlog/spdlog.h"

#include <memx/accl/MxAcclMT.h>

using namespace MX::Runtime;
using namespace MX::Types;
using namespace MX::Utils;

MxAcclMT::~MxAcclMT()
{

    // need to call model_manual_stop for all models
    for (auto &dfp_runner_pair : runner_table) {
        DFPRunner* dfp_runner = dfp_runner_pair.second;
        for (auto model : dfp_runner->models) {
            model->model_manual_stop();
        }
    }

    // base class's dtor will take care of the rest
}

bool MxAcclMT::send_input(std::vector<float*> in_data, int model_id, int stream_id, int32_t timeout)
{
    int dfp_id = 0; // TODO: temp solution

    // Get the DFP runner --> Model object
    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    DFPRunner* dfp_runner = it->second;
    MxModel* model = dfp_runner->models[model_id];
    lock.unlock();

    // Check if the model is running
    if (!model->model_manual_run.load()) {
        // do model_manual_start
        model->model_manual_start();
    }

    // call model_manual_send
    return model->model_manual_send(in_data, stream_id, timeout);
}

bool MxAcclMT::receive_output(std::vector<float*> &out_data, int model_id, int stream_id, int32_t timeout)
{
    int dfp_id = 0; // TODO: temp solution

    // Get the DFP runner --> Model object
    std::shared_lock<std::shared_mutex> lock(runner_mutex);
    auto it = runner_table.find(dfp_id);
    DFPRunner* dfp_runner = it->second;
    MxModel* model = dfp_runner->models[model_id];
    lock.unlock();

    // Check if the model is running
    if (!model->model_manual_run.load()) {
        // do model_manual_start
        model->model_manual_start();
    }

    // call model_manual_receive
    return model->model_manual_receive(out_data, stream_id, timeout);
}

