// Copyright (c) 2025 MemryX
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef DFP_EXECUTOR_H
#define DFP_EXECUTOR_H

#pragma once
#include <string>
#include <vector>
#include <atomic>
#include <deque>
#include <stack>
#include <thread>
#include <cstdint>
#include <cstring>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <mutex>
#include <shared_mutex>

#include "mxasio.hpp"

#include "contexts.h"

#include <memx/accl/messages.h>
#include <memx/accl/utils/mxTypes.h>
#include <memx/accl/utils/blocky_queue.h>


using mxasio::ip::tcp;

using namespace MX::RPC; // BlockyQueue

namespace MX
{
namespace Manager
{

// these tasks get queued up by the Scheduler thread
// into each Executor thread, which then runs them
struct ExecutorTask {
    ExecutorTask() : dfp_ctx(nullptr), allowed_device(-1), frame_limit(0), time_limit(0), stop_on_empty(false), freq_option(MX::Types::MxFrequencyOption::FREQ_USE_CONF) {}
    DFPContext* dfp_ctx;
    int allowed_device;
    uint64_t frame_limit;
    uint32_t time_limit;
    bool stop_on_empty;
    MX::Types::MxFrequencyOption freq_option;
};


// each MXA is like its own "DFP Executor"
//
// How we do schedule which device does which DFP?
//
// - based on the current mix of DFPs, etc., (Scheduler stuff; higher layers)
// - if a given DFP is marked for multiple devices, have
//   *multiple DFPExecutor instances* run on the *same* DFPContext
//   This is safe because lower layers (ifmap/ofmap queus & freelists)
//   are made to support potentially multiple executors accessing them
//   at a time.
//
//
// DFPExecutor should have a pair of I/O threads per each ModelContext
// contained within the DFPContext
//
// Execution happens for the given N number of frames (applies to all subModels)
// or after a given total amount of time ('max latency')
//
//
// Swapping DFPs happens here too -- when the DFPContext is changed, we shouldn't have
// to modify *anything* in our running Model I/O threads, thanks to the wonderful
// layers of abstraction in ModelContext->ClientMeta->buffers/freelists/queue, etc...
// - NOTE: you do need to add/remove I/O thread pairs if the incoming DFP has a
//         different number of sub-Models, though!

class ModelThreadPair
{
  public:
    ModelThreadPair();
    ~ModelThreadPair();

    // assigns given DFP
    // start running for N frames / M time
    bool assign_and_start(ExecutorTask* task, int model_id_);

    // halt while draining frame pipeline
    void halt();

    // forcibly halt; do not drain pipeline
    void force_halt();

    // current DFP ptr
    DFPContext* d;

    // publicly accessible "interrupt flags" that
    // the main DFPExecutor thread waits on

    // check out our synchronization mechanism here:
    // https://link.excalidraw.com/l/55syurcaaU1/A46Bz3AK1dp
    std::condition_variable s_done;
    std::condition_variable s_loop_ready;
    std::condition_variable s_enter_out;
    std::condition_variable s_enter_in;
    std::atomic_bool a_input_done;
    bool enter_out_flag = false;
    bool enter_in_flag = false;
    int num_loop_ready = 0;

    std::mutex m_flags;
    std::mutex m_ready;
    std::mutex m_enter_in;
    std::mutex m_enter_out;

    uint8_t driver_ctx_id;


    std::mutex* m_dumpster_lock;
    uint8_t* dumpster;

  private:
    // stored internally
    int model_id;
    uint64_t frame_limit;
    uint32_t time_limit;
    bool stop_on_empty;


    // targets of threads
    void input_loop();
    void output_loop();



    // 3'b000=run, 3'b010=halt, 3'b011=force-halt, 3'b101=terminate
    std::condition_variable s_stop;
    enum StopFlags : char {
        SF_RUN = 0,
        SF_HALT = 2,
        SF_FORCE_HALT = 3,
        SF_TERMINATE = 5
    };
    std::atomic<StopFlags> a_stop_in;
    std::atomic<StopFlags> a_stop_out;

    std::thread* ithread;
    std::thread* othread;

    BQExtFlag<ContextClient*>* inflights;

};


class DFPExecutor
{

  public:
    DFPExecutor(uint8_t device_id_, const std::vector<device_info_t>* devinfos_);
    ~DFPExecutor();

    bool run_dfp(ExecutorTask* task);

    // expand/contract # threads depending on # submodels
    void add_iothread_pair(int submodel_id, std::mutex* m_dumpster_lock, uint8_t* dumpster);
    void remove_iothread_pair(int submodel_id);

    // download and start stream to real hardware
    bool download_and_start_dfp(DFPContext* d);
    bool stop_dfp(DFPContext* d);

    // close all open contexts for this device
    bool close_device();

    // close only the given driver context ID
    bool close_ctx(uint8_t driver_ctx_id);

    // tracks open driver ctx IDs
    std::unordered_set<uint8_t> driver_ctx_set;

    // get max temps
    float avg_max_temp();
    float inst_max_temp();

    // get temps for each chip on this device (const ref to this->)
    const std::vector<float> &avg_all_temps();
    const std::vector<float>  inst_all_temps();

    // get powers (if possible)
    float avg_power();
    float inst_power();

    bool can_get_power; // whether this device can get power data

    uint8_t get_num_chips();

    // set the power mode for this device
    bool set_power_mode(MX::Types::MxFrequencyOption fop);

  private:
    bool open_device(DFPContext* d);
    uint8_t device_id;
    const std::vector<device_info_t>* devinfos;

    uint8_t num_chips; // number of chips on this device (set in open_device())

    std::map<int, ModelThreadPair*> thread_pairs;

    //--------------------------------------------------------------------------------

    // start/stop thread that monitor hardware temps & powers
    void start_hw_monitor();
    void stop_hw_monitor();

    // hardware monitor thread
    std::thread* hw_monitor_thread;

    // parses power config file
    void read_power_mode();
    uint16_t c4_freq;
    uint16_t c4_volt;
    uint16_t c2_freq;
    uint16_t c2_volt;


    // hardware monitor thread's loop function
    void hw_monitor_loop();
    bool hw_monitor_running;
    std::shared_mutex m_hw_monitor_lock;

    // the actual temp and power data retrieved
    // by the public functions above
    std::vector<float> _avg_temps; // average temps per chip
    float              _avg_power; // average power consumption

    // the 'instantaneous' temps and power are just the newest values
    // in the deques

    //--------------------------------------------------------------------------------

    // interval to poll temp and power
    // default: 4 per second
    std::chrono::milliseconds hw_monitor_interval = std::chrono::milliseconds(250);

    // number of samples to average over
    // (time the avg is over is thus hw_monitor_interval * hw_monitor_avg_samples)
    unsigned int hw_monitor_avg_samples = 12; // 3 seconds

    // the complete window of powers so far
    std::deque<float> power_window;

    // for temps, it's per interval * per chip
    std::vector<std::deque<float>> temp_window;

    mutable std::shared_mutex m_temp_power; // mutex for temp and power data

    //--------------------------------------------------------------------------------

    uint8_t* dumpster; // buffer for dumping overflow frames
    size_t  dumpster_size; // size of the dumpster buffer
    mutable std::mutex m_dumpster_lock; // mutex for changing dumpster size

    //--------------------------------------------------------------------------------

    // need mutex all memx_get_feature() calls because of the driver
    mutable std::shared_mutex m_memx_get_feature;
    mutable std::mutex m_driver_ctx_set;
};




}
}

#endif // DFP_EXECUTOR_H
