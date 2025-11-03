// Copyright (c) 2025 MemryX
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef MX_MODEL
#define MX_MODEL

#pragma once
#include <iostream>
#include <vector>
#include <stdint.h>
#include <cstdio>
#include <thread>
#include <functional>
#include <cstring>
#include <unordered_set>
#include <filesystem>
#include <array>

#include <memx/memx.h>

#include <memx/accl/dfp.h>
#include <memx/accl/client.h>
#include <memx/accl/prepost.h>
#include <memx/accl/utils/blocky_queue.h>
#include <memx/accl/utils/thread_pool.h>
#include <memx/accl/utils/featureMap.h>
#include <memx/accl/utils/errors.h>
#include <memx/accl/utils/mxTypes.h>
#include <memx/accl/utils/locked_var.h>

using namespace std;
using namespace MX::Utils;

namespace MX
{
namespace Runtime
{
/**
 * Base Model class that needs to be inherited by Model class
 * This class provides virtual funtions required by user that are
 * supposed to be overriden by child classes
*/
class ModelBase
{
  public:
    //function refernce for callback functions
    typedef std::function<bool(vector<const MX::Types::FeatureMap*>, int)> float_callback_t;

    //connect_stream to this Model
    void connect_stream(float_callback_t, float_callback_t, int);

    //Set number of workers
    void set_num_workers(int, int);
    // Set multi-thread FMap conversion threads
    void set_parallel_fmap_convert(int);
    //Start the model
    void model_start();

    //Stop the model
    void model_stop();

    // manual threading model start function to init model and featureMaps
    void model_manual_start();

    // // manual threading model stop function to init model and featureMaps
    void model_manual_stop();

    // manual threading model send for float
    bool model_manual_send(std::vector<float*>, int, int32_t);

    // manual threadin send for float
    bool model_manual_receive(std::vector<float*> &, int, int32_t);
    //Get num streams in this model
    int get_num_streams();

    //Wait for model to finish
    void model_wait();

    MX::Types::MxModelInfo return_model_info();

    MX::Types::MxModelInfo return_pre_model_info();

    MX::Types::MxModelInfo return_post_model_info();

    void model_set_post(std::filesystem::path post_model_path, const std::vector<size_t> &);

    void model_set_pre(std::filesystem::path pre_model_path);

    bool manual_run(std::vector<float*>, std::vector<float*> &, int, int32_t);

    // void log_model_info(){throw runtime_error("base print info is called");};

    ~ModelBase() {};

};

class MxModel : public ModelBase
{
  private:
    int model_id_; // unique id of the model on MXA
    int group_id_; // unique id of MXA
    int num_streams_;// num streams connected to this model
    Dfp::DfpObject* dfp_; // Dfp object
    MX::Utils::BlockyQueue<int> stream_queue; //queue to store stream ids for ifmaps

    std::array<bool, 2> use_model_shape;

    vector<uint8_t> in_ports_; // input port information
    vector<uint8_t> out_ports_; // output port information
    int input_num_workers_;
    int output_num_workers_;
    thread_pool* input_pool;
    thread_pool* output_pool;
    void model_send_fun(); //send thread function to perform ifmap
    void model_recv_fun(); //recv thread function to perform ofmap
    thread* model_send_thread; //send thread for model
    thread* model_recv_thread; //recv thread for model
    thread* model_manual_recv_thread; //recv thread for model
    void model_manual_recv_fun(); //recv thread function to perform ofmap
    typedef std::function<bool(vector<const MX::Types::FeatureMap*>, int stream_id)> combined_input_callback_t;
    typedef std::function<bool(vector<const MX::Types::FeatureMap*>, int stream_id)> combined_output_callback_t;
    //Task done by each worker of input threadpool
    bool inputTask(combined_input_callback_t in_cb, vector<const MX::Types::FeatureMap*>inputs, int stream, int stream_idx);
    //Task done by each worker of output threadpool
    bool outputTask(combined_output_callback_t out_cb, vector<const MX::Types::FeatureMap*>outputs, int stream, int stream_idx);
    //vector of input callback functions
    vector<combined_input_callback_t> comb_in_call;
    //vector of output callback functions
    vector<combined_output_callback_t> comb_out_call;
    //set of stream ids connected to the whole accl
    unordered_set<int> stream_set_;
    //list of streamids connected to the model
    vector<int> stream_id_list;

    //Vector of featureMaps of size num_streams that holds inputs for pre-processing models
    vector<vector<MX::Types::FeatureMap*>> pre_in_featuremaps_;
    //Vector of featureMaps of size num_streams that holds inputs for models
    vector<vector<MX::Types::FeatureMap*>> in_featuremaps_;
    //Vector of featureMaps of size num_streams that holds outputs for models
    vector<vector<MX::Types::FeatureMap*>> out_featuremaps_;
    //Vector of featureMaps of size num_streams that holds inputs for post-processing models
    vector<vector<MX::Types::FeatureMap*>> post_out_featuremaps_;

    //model information
    MX::Types::MxModelInfo model_info;
    MX::Types::MxModelInfo pre_model_info;
    MX::Types::MxModelInfo post_model_info;

    PrePost* pre_info_model;
    PrePost* post_info_model;
    float mxa_gen;//Generation of chip DFP is compiled

    vector<MX::Types::FeatureMap*> single_input_featuremap_;
    vector<MX::Types::FeatureMap*> single_output_featuremap_;
    std::condition_variable model_manual_cv;
    std::mutex manual_mutex;
    bool model_manual_in_done;

    //input task
    bool input_task_flag;
    std::mutex input_task_mutex;
    std::condition_variable input_task_cv;

    LockedVar<int> input_thread_counter;
    std::mutex input_thread_mutex;
    std::condition_variable input_thread_cv;

    //model_recv
    bool model_recv_flag;
    std::mutex model_recv_mutex;
    std::condition_variable model_recv_cv;

    vector<std::mutex*> out_task_mutex;
    vector<std::condition_variable*> out_task_cv;

    std::vector<int> open_contexts;

    bool local_mode;
    Client* client;

    int context_send_current_index = 0;
    int number_of_contexts;
    int out_frame_cnt = 0;
    int in_frame_cnt = 0;

    //Queue to pass stream id and context id from send to recv functions
    MX::Utils::BlockyQueue<std::pair<int, int>> pair_stream_context_queue;


    //Pre-processing model items
    std::filesystem::path post_model_path_;
    std::vector<PrePost*> post_model;
    std::vector<std::vector<MX::Types::FeatureMap*>> out_featuremaps_pptemp_;
    std::vector<std::vector<MX::Types::FeatureMap*>> transposed_out_featuremaps_;
    std::vector<size_t> post_out_size;

    //Post-processing model items
    std::filesystem::path pre_model_path;
    std::vector<PrePost*> pre_model;
    std::vector<std::vector<MX::Types::FeatureMap*>> in_featuremaps_pptemp_;
    std::vector<std::vector<MX::Types::FeatureMap*>> transposed_in_featuremaps_;
    std::vector<size_t> pre_out_size;

    void create_and_append_in_fm();
    void create_and_append_out_fm();

    std::unordered_map<int, int> stream_id_map_;
    std::mutex fm_create_mutex;
    std::mutex manual_mutex_in;
    std::mutex manual_mutex_out;
    std::vector<std::mutex*> manual_recv_mutex;
    std::vector<std::condition_variable*> manual_recv_cv;
    std::vector<bool> manual_recv_flag;
    std::vector<std::mutex*> manual_recv_task_mutex;
    std::vector<std::condition_variable*> manual_recv_task_cv;
    std::vector<bool> manual_recv_task_flag;
    std::mutex manual_init_mutex;
    std::condition_variable manual_init_cv;
    void create_append_manual_mem();

    int parallel_fmap_convert_threads;

    Dfp::DfpMeta* meta_;

    void _post_inference(int stream);
    void _pre_inference(int stream);
    void _pre_copy(int stream);

    void _recv_wait(int stream);
    void _send_wait(int stream);
    void _manual_recv_wait(int stream);

  public:
    MxModel(int model_id, Dfp::DfpObject* dfp_object,
            std::array<bool, 2> use_model_shape_, bool local_mode_,
            std::vector<int> popen_contexts, Client* client_ = nullptr); // Construct model for Inference

    void model_start();
    void model_stop();
    void model_wait();

    LockedVar<bool> model_run; // flag to specify if model is running
    LockedVar<bool> model_recv_run; // flag to specify if model recv thread is running
    LockedVar<bool> model_manual_run; // flag to specify manual threadin is opted out

    void model_manual_start();
    void model_manual_stop();
    ~MxModel();
    void connect_stream(combined_input_callback_t in_cb, combined_output_callback_t out_cb, int stream_id);
    int get_num_streams();
    // void log_model_info();
    MX::Types::MxModelInfo return_model_info();
    MX::Types::MxModelInfo return_pre_model_info();
    MX::Types::MxModelInfo return_post_model_info();

    bool model_manual_send(std::vector<float*> in_data, int stream_id, int32_t timeout = 0);

    bool model_manual_receive(std::vector<float*> &out_data, int stream_id, int32_t timeout = 0);

    bool manual_run(std::vector<float*> in_data, std::vector<float*> &out_data, int pstream_id, int32_t timeout = 0);

    void model_set_post(std::filesystem::path post_model_path, const std::vector<size_t> &post_out_size_list);

    void model_set_pre(std::filesystem::path pre_model_path);
    void set_num_workers(int input_workers, int output_workers);
    void set_parallel_fmap_convert(int num_threads);
};
} // namespace Runtime
} // namespace MX

#endif
