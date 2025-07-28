// Copyright (c) 2025 MemryX
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memx/memx.h>

#include "spdlog/spdlog.h"
#ifdef _WIN32
#include <spdlog/sinks/win_eventlog_sink.h>
#endif

#include "dfp_executor.h"
#include "color_print.h"

using mxasio::ip::tcp;

using namespace MX::Manager;
using namespace MX::RPC;

// DFP Executor
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
DFPExecutor::DFPExecutor(uint8_t device_id_, const std::vector<device_info_t>* devinfos_) : devinfos(devinfos_)
{
    device_id = device_id_;

    num_chips = devinfos->at(device_id).chip_count;
    can_get_power = devinfos->at(device_id).can_get_power_data;

    // initialize my module to default freq/volt options
    read_power_mode();
    set_power_mode(MX::Types::MxFrequencyOption::FREQ_USE_CONF);

    hw_monitor_running = false;
    hw_monitor_thread = nullptr;

    // initialize the thread pairs map
    thread_pairs.clear();

    // clear dumpsters
    dumpster = nullptr;
    dumpster_size = 0;

    spdlog::info("DFPExecutor {}: created DFPExecutor for device ID {} with num chips {} and can_get_power {}",
                 device_id, device_id, num_chips, can_get_power);
}

DFPExecutor::~DFPExecutor()
{

    // stop all the thread pairs
    for(auto it = thread_pairs.begin(); it != thread_pairs.end(); it++) {
        ModelThreadPair* pair = it->second;
        delete pair; // sets SF_TERMINATE and kills threads
    }

    stop_hw_monitor();

    // clear the map
    thread_pairs.clear();

    {
        std::lock_guard<std::mutex> lock(m_dumpster_lock);
        if(dumpster != nullptr) {
            delete [] dumpster;
            dumpster = nullptr;
        }
    }
}


//---------------------------------------------------------
//---------------------------------------------------------

void DFPExecutor::start_hw_monitor()
{
    std::unique_lock<std::shared_mutex> lock(m_hw_monitor_lock);
    if(hw_monitor_running == false) {
        hw_monitor_running = true;
        hw_monitor_thread = new std::thread(&DFPExecutor::hw_monitor_loop, this);
        spdlog::debug("DFPExecutor {}: started hardware monitor thread", device_id);
    }
    else {
        //spdlog::error("DFPExecutor {}: hardware monitor thread already running", device_id);
    }
    lock.unlock();
}

void DFPExecutor::stop_hw_monitor()
{
    std::shared_lock<std::shared_mutex> lock(m_hw_monitor_lock);
    if(hw_monitor_running == true) {
        hw_monitor_running = false;
        if(hw_monitor_thread != nullptr) {
            if(hw_monitor_thread->joinable()) {
                hw_monitor_thread->join();
            }
            delete hw_monitor_thread;
            hw_monitor_thread = nullptr;
            spdlog::debug("DFPExecutor {}: stopping hardware monitor thread", device_id);
        }
    }
    else {
        //spdlog::info("DFPExecutor {}: hardware monitor thread not running", device_id);
    }
    lock.unlock();
}

void DFPExecutor::hw_monitor_loop()
{

    // initial timestamp
    std::chrono::steady_clock::time_point base_time = std::chrono::steady_clock::now();

    std::vector<uint64_t> tvalues(num_chips, 0);

    {
        std::unique_lock<std::shared_mutex> lock(m_temp_power);
        temp_window.resize(num_chips);
        _avg_temps.resize(num_chips);

        // clear power window
        power_window.clear();

        // clear each temp_window
        for(uint8_t i = 0; i < num_chips; i++) {
            temp_window[i].clear();
        }
    }

    uint64_t pvalue = 0;

    std::shared_lock<std::shared_mutex> hlock(m_hw_monitor_lock, std::defer_lock);

    for(;;) {

        // sleep for base_time + interval
        std::this_thread::sleep_until(base_time + hw_monitor_interval);

        hlock.lock();
        if(hw_monitor_running == false) {
            hlock.unlock();
            spdlog::debug("DFPExecutor {}: hardware monitor thread stopped", device_id);
            return; // die
        }
        else {

            // get temp of each chip
            for(uint8_t i = 0; i < num_chips; i++) {
                memx_status status = memx_get_feature(device_id, i, OPCODE_GET_TEMPERATURE, &(tvalues[i]));
                if(UNLIKELY(memx_status_error(status))) {
                    spdlog::error("DFPExecutor {}: memx_get_chip_temp() failed with error {} for chip {}", device_id, (uint32_t)status, i);
                    tvalues[i] = 0; // set to 0 on error
                    hlock.unlock();
                    return; // die
                }
            }

            // get power value
            if(can_get_power) {
                memx_status status = memx_get_feature(device_id, 0, OPCODE_GET_POWER, &pvalue);
                if(UNLIKELY(memx_status_error(status))) {
                    spdlog::error("DFPExecutor {}: memx_get_power() failed with error {}", device_id, (uint32_t)status);
                    pvalue = 0; // set to 0 on error
                    hlock.unlock();
                    return; // die
                }
            }

            // acquire exclusive lock and update values
            {
                std::unique_lock<std::shared_mutex> lock(m_temp_power);

                if(can_get_power) {
                    power_window.push_back(static_cast<float>(pvalue));
                    if(power_window.size() > hw_monitor_avg_samples) {
                        power_window.pop_front(); // remove oldest sample
                    }
                }

                for(uint8_t i = 0; i < num_chips; i++) {
                    temp_window[i].push_back(static_cast<float>(tvalues[i]) - 273.0); // convert Kelvin to Celsius
                    if(temp_window[i].size() > hw_monitor_avg_samples) {
                        temp_window[i].pop_front(); // remove oldest sample
                    }
                }

                // update the averages
                _avg_power = 0.0f;
                if(can_get_power) {
                    for(unsigned int j = 0; j < power_window.size(); j++) {
                        _avg_power += power_window[j];
                    }
                    if(!power_window.empty()) {
                        _avg_power /= static_cast<float>(power_window.size());
                    }
                }

                for(uint8_t i = 0; i < num_chips; i++) {
                    _avg_temps[i] = 0.0f;
                    for(unsigned int j = 0 ; j < temp_window[i].size(); j++) {
                        _avg_temps[i] += temp_window[i][j];
                    }
                    if(!temp_window[i].empty()) {
                        _avg_temps[i] /= static_cast<float>(temp_window[i].size());
                    }
                }
            }

            hlock.unlock();
        }

        // update the base time for the next iteration
        base_time = std::chrono::steady_clock::now();

    }

    spdlog::debug("DFPExecutor {}: hardware monitor thread stopped", device_id);

}

//---------------------------------------------------------

// returns avg_temps
const std::vector<float> &DFPExecutor::avg_all_temps()
{
    std::shared_lock<std::shared_mutex> lock(m_temp_power);
    return _avg_temps;
}

// returns a vector (each elem is a chip) at the back (most recent) of the temp_window deque
const std::vector<float> DFPExecutor::inst_all_temps()
{
    std::shared_lock<std::shared_mutex> lock(m_temp_power);
    std::vector<float> inst_temps;
    for(const auto &temp_deque : temp_window) {
        if(!temp_deque.empty()) {
            inst_temps.push_back(temp_deque.back());
        }
        else {
            inst_temps.push_back(0.0f); // if empty, return 0.0
        }
    }
    return inst_temps;
}

// returns the maximum value of the avg_temps vector
float DFPExecutor::avg_max_temp()
{
    std::shared_lock<std::shared_mutex> lock(m_temp_power);
    float max_temp = -3000.0f;
    for(const auto &temp : _avg_temps) {
        if(temp > max_temp) {
            max_temp = temp;
        }
    }
    return max_temp;
}

// returns the maximum value of the back of the temp_window deque (most recent)
float DFPExecutor::inst_max_temp()
{
    std::shared_lock<std::shared_mutex> lock(m_temp_power);
    float max_temp = -3000.0f;
    for(const auto &temp_deque : temp_window) {
        if(!temp_deque.empty()) {
            if(temp_deque.back() > max_temp) {
                max_temp = temp_deque.back();
            }
        }
    }
    return max_temp;
}

// returns the average power value
float DFPExecutor::avg_power()
{
    std::shared_lock<std::shared_mutex> lock(m_temp_power);
    return _avg_power;
}

// returns the instantaneous power value (most recent in deque)
float DFPExecutor::inst_power()
{
    std::shared_lock<std::shared_mutex> lock(m_temp_power);
    if(!power_window.empty()) {
        return power_window.back();
    }
    else {
        return 0.0f; // if empty, return 0.0
    }
}


uint8_t DFPExecutor::get_num_chips()
{
    return num_chips;
}

//---------------------------------------------------------

void DFPExecutor::read_power_mode()
{
#ifdef _WIN32
    std::string config_path = "C:\\Program Files\\memryx\\power.conf";
#else
    std::string config_path = "/etc/memryx/power.conf";
#endif

    if(std::filesystem::exists(config_path)) {
        // read each line
        std::ifstream fd(config_path);
        for( std::string line; getline( fd, line ); ) {
            if(line[0] == '#') {
                continue;
            }
            std::string varname = line.substr(0, 6);
            if(varname == "FREQ4C") {
                std::string val = line.substr(7, 3);
                c4_freq = (uint16_t) std::stoi(val);
            }
            else if(varname == "VOLT4C") {
                std::string val = line.substr(7, 3);
                c4_volt = (uint16_t) std::stoi(val);
            }
            else if(varname == "FREQ2C") {
                std::string val = line.substr(7, 3);
                c2_freq = (uint16_t) std::stoi(val);
            }
            else if(varname == "VOLT2C") {
                std::string val = line.substr(7, 3);
                c2_volt = (uint16_t) std::stoi(val);
            }

        }
    }
    else {
        // set default values
        c4_freq = 600;
        c4_volt = 700;
        c2_freq = 600;
        c2_volt = 700;
    }
}


bool DFPExecutor::set_power_mode(MX::Types::MxFrequencyOption fop)
{
    memx_status status;

    if(fop == MX::Types::MxFrequencyOption::FREQ_USE_CONF) {
        if(num_chips == 2) {
            status = memx_set_feature(device_id, 0, OPCODE_SET_FREQUENCY, c2_freq);
            status = memx_set_feature(device_id, 1, OPCODE_SET_FREQUENCY, c2_freq);
            status = memx_set_feature(device_id, 0, OPCODE_SET_VOLTAGE,   c2_volt);
        }
        else if(num_chips >= 4) {
            // all num_chips >= 4 use the c4 values
            for(int i = 0; i < num_chips; i++) {
                status = memx_set_feature(device_id, i, OPCODE_SET_FREQUENCY, c4_freq);
            }
            status = memx_set_feature(device_id, 0, OPCODE_SET_VOLTAGE, c4_volt);
        }
        else {
            spdlog::error("DFPExecutor {}: num_chips {} is invalid for set_power_mode()", device_id, num_chips);
            return false;
        }
    }
    else {
        MX::Types::MxVoltageOption volt = MX::Types::getVoltageFromFrequency(fop);

        // set all chips to the same frequency that's passed in fop
        for(int i = 0; i < num_chips; i++) {
            status = memx_set_feature(device_id, i, OPCODE_SET_FREQUENCY, fop);
        }
        status = memx_set_feature(device_id, 0, OPCODE_SET_VOLTAGE, volt);
    }

    return memx_status_no_error(status);
}



//---------------------------------------------------------
//---------------------------------------------------------


bool DFPExecutor::close_device()
{

    stop_hw_monitor();

    std::unique_lock<std::mutex> dcslock(m_driver_ctx_set);
    for(uint8_t driver_ctx_id : driver_ctx_set) {
        memx_status status = memx_set_abort_read(driver_ctx_id);
        if(memx_status_error(status)) {
            spdlog::error("DFPExecutor {}: memx_set_abort_read() failed with error {} for driver ctx {}", device_id, (uint32_t)status, driver_ctx_id);
            return false;
        }

        // close the device for each driver context
        status = memx_close(driver_ctx_id);
        if(memx_status_error(status)) {
            spdlog::critical("DFPExecutor {}: memx_close() failed with error {} for driver ctx {}", device_id, (uint32_t)status, driver_ctx_id);
            return false;
        }
        spdlog::debug("DFPExecutor {}: closed driver context {}", device_id, driver_ctx_id);
    }

    driver_ctx_set.clear();
    dcslock.unlock();

    _avg_temps.clear();
    _avg_power = 0.0f;
    power_window.clear();
    for(uint8_t i = 0; i < temp_window.size(); i++) {
        temp_window[i].clear();
    }
    temp_window.clear();

    spdlog::debug("DFPExecutor {}: closed all driver contexts and cleared temp/power data", device_id);
    return true;
}


bool DFPExecutor::close_ctx(uint8_t driver_ctx_id)
{

    // check if this ctx is open
    std::unique_lock<std::mutex> dcslock(m_driver_ctx_set);
    if(driver_ctx_set.count(driver_ctx_id) == 0) {
        spdlog::warn("DFPExecutor {}: close_ctx() called for driver context {} but it is not open", device_id, driver_ctx_id);
        return true; // nothing to close
    }

    memx_status status = memx_close(driver_ctx_id);
    if(memx_status_error(status)) {
        spdlog::critical("DFPExecutor {}: memx_close() failed with error {} for driver ctx {}", device_id, (uint32_t)status, driver_ctx_id);
        return false;
    }

    driver_ctx_set.erase(driver_ctx_id);
    spdlog::debug("DFPExecutor {}: closed driver context {}", device_id, driver_ctx_id);
    return true;
}



bool DFPExecutor::open_device(DFPContext* d)
{

    uint8_t driver_ctx_id = d->device2context_table[device_id];
    std::unique_lock<std::mutex> dcslock(m_driver_ctx_set);
    // skip if already open
    if(driver_ctx_set.count(driver_ctx_id) > 0) {
        spdlog::debug("DFPExecutor {}: open_device() called but driver context {} is already open", device_id, driver_ctx_id);
        return true;
    }

    memx_status status = memx_open(driver_ctx_id, device_id, MEMX_DEVICE_CASCADE_PLUS);
    if(memx_status_error(status)) {
        spdlog::critical("DFPExecutor {}: memx_open() failed with error {} for driver ctx {}", device_id, (uint32_t)status, driver_ctx_id);
        return false;
    }

    start_hw_monitor(); // just returns if already running

    driver_ctx_set.insert(driver_ctx_id);
    spdlog::debug("DFPExecutor {}: opened driver context {}", device_id, driver_ctx_id);
    return true;
}

bool DFPExecutor::download_and_start_dfp(DFPContext* d)
{
    if(UNLIKELY(d == nullptr)) {
        spdlog::critical("DFPExecutor {}: download_and_start_dfp() called with nullptr DFPContext", device_id);
        return false;
    }

    uint8_t driver_ctx_id = d->device2context_table[device_id];
    std::unique_lock<std::mutex> dcslock(m_driver_ctx_set);
    // check that this ctx id is open
    bool isnt_open = (driver_ctx_set.count(driver_ctx_id) == 0);
    dcslock.unlock();
    if(isnt_open) {
        // not yet, so open it now
        if(open_device(d) == false) {
            spdlog::error("DFPExecutor {}: download_and_start_dfp() failed to open device for driver context {}", device_id, driver_ctx_id);
            return false;
        }
    }

    // print all the members of the DfpContext from d->dfp_obj->get_cache()
    Dfp::pDfpContext cache = d->dfp_obj->get_cache();
    if(UNLIKELY(cache == nullptr)) {
        spdlog::critical("DFPExecutor {}: download_and_start_dfp() called with nullptr DFPContext cache", device_id);
        return false;
    }

    // do the above printing, but with spdlog::debug
    spdlog::debug("DFPExecutor {}: DFPContext cache:", device_id);
    spdlog::debug("  identifier_data: {}", cache->identifier_data);
    spdlog::debug("  dfp_attr: {}", cache->dfp_attr);
    spdlog::debug("  input_mode_flag: {}", cache->input_mode_flag);
    spdlog::debug("  input_port_number: {}", cache->input_port_number);
    spdlog::debug("  output_port_number: {}", cache->output_port_number);
    spdlog::debug("  weight_size: {}", cache->weight_size);
    spdlog::debug("  config_size: {}", cache->config_size);
    spdlog::debug("  pInputConfigList: {}", static_cast<void*>(cache->pInputConfigList));
    spdlog::debug("  pOuputConfigList: {}", static_cast<void*>(cache->pOuputConfigList));
    spdlog::debug("  pWeightBaseAdr: {}", static_cast<void*>(cache->pWeightBaseAdr));
    spdlog::debug("  pRgCfgBaseAdr: {}", static_cast<void*>(cache->pRgCfgBaseAdr));
    spdlog::debug("  DFPExecutor {}: iport_size[0]: {}", device_id, d->info->port_info[0]->iport_sizes[0]);
    spdlog::debug("  DFPExecutor {}: oport_size[0]: {}", device_id, d->info->port_info[0]->oport_sizes[0]);

    // config the dumpster
    {
        std::lock_guard<std::mutex> lock(m_dumpster_lock);
        if(dumpster == nullptr || dumpster_size < d->info->biggest_ofmap_bytes) {
            // delete old dumpster if it exists
            if(dumpster != nullptr) {
                delete [] dumpster;
            }
            // allocate new dumpster
            dumpster_size = d->info->biggest_ofmap_bytes;
            dumpster = new uint8_t[dumpster_size];
        }
    }

    // download the DFP to the device
    memx_status status = memx_download_model_from_cahce(driver_ctx_id, d->dfp_obj->get_cache(), 0,
                         MEMX_DOWNLOAD_TYPE_WTMEM_AND_MODEL);
    if(UNLIKELY(memx_status_error(status))) {
        spdlog::critical("DFPExecutor {}: memx_download_model_from_cache() failed with error {} for driver ctx {}",
                     device_id, (uint32_t)status, driver_ctx_id);
        return false;
    }

    status = memx_set_stream_enable(driver_ctx_id, 0);
    if(UNLIKELY(memx_status_error(status))) {
        spdlog::critical("DFPExecutor {}: memx_set_stream_enable() failed with error {} for driver ctx {}",
                     device_id, (uint32_t)status, driver_ctx_id);
        return false;
    }

    spdlog::debug("DFPExecutor {}: started DFP with hash {}", device_id, MX::sha512::to_base64(d->hash).c_str());
    //d->print_clients();
    return true;
}


bool DFPExecutor::stop_dfp(DFPContext* d)
{

    uint8_t driver_ctx_id = d->device2context_table[device_id];
    std::unique_lock<std::mutex> dcslock(m_driver_ctx_set);
    if(UNLIKELY(driver_ctx_set.count(driver_ctx_id) == 0)) {
        // not open, so nothing to stop
        spdlog::warn("DFPExecutor {}: stop_dfp() called but driver context {} is not open", device_id, driver_ctx_id);
        return true;
    }

    // NOTE:
    // We have to set `wait` to one to ensure the function waits for `ifmap` and `ofmap` to complete.
    // Setting `wait` to zero returns immediately, which may cause a hang in the next round of download_dfp
    // if `ifmap` or `ofmap` are still processing from the previous operation.
    int wait = 1;
    memx_status status = memx_set_stream_disable(driver_ctx_id, wait);
    if(UNLIKELY(memx_status_error(status))) {
        spdlog::error("DFPExecutor {}: memx_set_stream_disable() failed with error {}", device_id, (uint32_t) status);
        return false;
    }
    spdlog::debug("DFPExecutor {}: stopped DFP", device_id);
    return true;
}


bool DFPExecutor::run_dfp(ExecutorTask* task)
{

    DFPContext* d = task->dfp_ctx;
    int n_models = d->info->num_models;
    uint8_t driver_ctx_id = d->device2context_table[device_id];

    spdlog::debug("DFPExecutor {}: running DFP with hash {} for {} frames, {} ms timeout",
                 device_id, MX::sha512::to_base64(d->hash).c_str(), task->frame_limit, task->time_limit);

    // download the DFP to the device
    if(download_and_start_dfp(d) == false) {
        spdlog::critical("DFPExecutor {}: failed to download and start DFP", device_id);
        return false;
    }

    for(int i = 0; i < n_models; i++) {
        // add a thread pair for each submodel
        // Note: skips IDs that already exist
        add_iothread_pair(i, &m_dumpster_lock, dumpster);
    }

    // now assign the DFPContext and model id to each thread pair
    for(int i = 0; i < n_models; i++) {

        // get the thread pair
        ModelThreadPair* pair = thread_pairs[i];

        // set the pair's driver_ctx_id
        pair->driver_ctx_id = driver_ctx_id;

        // assign the DFPContext and model id
        if(pair->assign_and_start(task, i) == false) {
            // failed to assign, throw error
            spdlog::error("DFPExecutor {}: failed to assign DFPContext to ModelThreadPair for model {}", device_id, i);
            return false;
        }
    }

    // now wait for all threads to finish
    for(int i = 0; i < n_models; i++) {

        ModelThreadPair* p = thread_pairs[i];

        // wait for input and output loop done
        std::unique_lock<std::mutex> tlock(p->m_ready);
        p->s_done.wait(tlock, [p] { return (p->num_loop_ready == 0); });
    }

    spdlog::debug("DFPExecutor {}: all threads finished", device_id);

    // stop the DFP
    if(stop_dfp(d) == false) {
        spdlog::error("DFPExecutor {}: failed to stop DFP", device_id);
        return false;
    }

    spdlog::debug("DFPExecutor {}: stopped DFP with hash {}. Returning true..", device_id, MX::sha512::to_base64(d->hash).c_str());

    // success
    return true;
}

//---------------------------------------------------------

void DFPExecutor::add_iothread_pair(int submodel_id, std::mutex* m_dumpster_lock, uint8_t* dumpster)
{

    if(thread_pairs.count(submodel_id) > 0) {
        // already exists, no need to add again
        return;
    }

    // create a new ModelThreadPair and assign it to the DFPContext
    ModelThreadPair* pair = new ModelThreadPair();
    pair->m_dumpster_lock = m_dumpster_lock;
    pair->dumpster = dumpster;

    // add to the map
    thread_pairs[submodel_id] = pair;
}


void DFPExecutor::remove_iothread_pair(int submodel_id)
{

    if(thread_pairs.count(submodel_id) == 0) {
        // doesn't exist, nothing to remove
        return;
    }

    // get the pair
    ModelThreadPair* pair = thread_pairs[submodel_id];

    // delete the pair
    delete pair;

    // remove from the map
    thread_pairs.erase(submodel_id);

}






// ModelThreadPair
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------

ModelThreadPair::ModelThreadPair()
{
    d = nullptr;
    driver_ctx_id = 0xFF; // invalid

    m_dumpster_lock = nullptr;
    dumpster = nullptr;

    a_stop_out.store(SF_RUN, std::memory_order_release);
    TSAN_RELEASE(&a_stop_out);
    a_stop_in.store(SF_RUN, std::memory_order_release);
    TSAN_RELEASE(&a_stop_in);

    a_input_done.store(true, std::memory_order_release);

    inflights = new BQExtFlag<ContextClient*>(UINT_MAX, &a_input_done, true);

    ithread = new std::thread(&ModelThreadPair::input_loop, this);
    othread = new std::thread(&ModelThreadPair::output_loop, this);
}


ModelThreadPair::~ModelThreadPair()
{

    {
        std::unique_lock<std::mutex> tlock(m_flags);
        a_stop_out.store(SF_TERMINATE, std::memory_order_release); // terminate
        TSAN_RELEASE(&a_stop_out);
        a_stop_in.store(SF_TERMINATE, std::memory_order_release); // terminate
        TSAN_RELEASE(&a_stop_in);
        s_stop.notify_all(); // wake up the threads
    }

    // wait for them to finish
    if(ithread->joinable()) {
        ithread->join();
    }
    if(othread->joinable()) {
        othread->join();
    }

    delete ithread;
    delete othread;

    delete inflights;
}


bool ModelThreadPair::assign_and_start(ExecutorTask* task, int model_id_)
{

    if(a_input_done.load(std::memory_order_acquire) == false) {
        spdlog::critical("ModelThreadPair {}-{}: input done flag is not set, this should not happen!", driver_ctx_id, model_id_);
        return false;
    }

    // assign this dfp and sub model id
    model_id = model_id_;
    d = task->dfp_ctx;

    frame_limit = task->frame_limit;
    time_limit = task->time_limit;
    stop_on_empty = task->stop_on_empty;
    a_input_done.store(false, std::memory_order_release); // reset input done flag

    a_stop_out.store(SF_RUN, std::memory_order_release);
    TSAN_RELEASE(&a_stop_out);
    a_stop_in.store(SF_RUN, std::memory_order_release);
    TSAN_RELEASE(&a_stop_in);

    spdlog::debug("ModelThreadPair {}-{}: assigned DFPContext with hash {}", driver_ctx_id, model_id, MX::sha512::to_base64(d->hash).c_str());

    // notify the input/output threads to start running
    {
        std::lock_guard<std::mutex> tlock(m_ready);

        if(num_loop_ready != 0) {
            spdlog::critical("ModelThreadPair {}-{}: num_loop_ready is not 0, this should not happen!", driver_ctx_id, model_id);
            return false;
        }

        num_loop_ready = 2; // both threads are ready
        s_loop_ready.notify_all(); // notify the threads
    }


    return true;
}


void ModelThreadPair::halt()
{

    // set the stop flag to 'soft' halt
    {
        std::unique_lock<std::mutex> tlock(m_flags);
        a_stop_out.store(SF_HALT, std::memory_order_release); // soft halt
        TSAN_RELEASE(&a_stop_out);
        a_stop_in.store(SF_HALT, std::memory_order_release); // soft halt
        TSAN_RELEASE(&a_stop_in);
        inflights->notify(); // wake up the inflight tracker
        s_stop.notify_all(); // wake up the threads
        inflights->notify(); // wake up the inflight tracker
    }


    // set the DFPContext to nullptr
    {
        std::unique_lock<std::mutex> dlock(m_flags);
        d = nullptr; // this will wake up the threads
    }
}


void ModelThreadPair::force_halt()
{

    // set the stop flag to 'soft' halt
    {
        std::unique_lock<std::mutex> tlock(m_flags);
        a_stop_out.store(SF_FORCE_HALT, std::memory_order_release); // force halt
        TSAN_RELEASE(&a_stop_out);
        a_stop_in.store(SF_FORCE_HALT, std::memory_order_release); // force halt
        TSAN_RELEASE(&a_stop_in);
        inflights->notify(); // wake up the inflight tracker
        s_stop.notify_all(); // wake up the threads
        inflights->notify(); // wake up the inflight tracker
        tlock.unlock();
    }


    // set the DFPContext to nullptr
    {
        std::unique_lock<std::mutex> dlock(m_flags);
        d = nullptr; // this will wake up the threads
    }
}


void ModelThreadPair::input_loop()
{

    for(;;) {

        // wait for both input and output threads to get ready
        std::unique_lock<std::mutex> dlock(m_ready);
        s_loop_ready.wait(dlock, [this] { return (num_loop_ready == 2) || (a_stop_in.load(std::memory_order_consume) == SF_TERMINATE); });
        TSAN_ACQUIRE(&a_stop_in);
        if(num_loop_ready < 2) {
            // if we got here, it means the program is being shutdown
            // by the main thread somehwere, so we should exit this loop
            dlock.unlock();
            spdlog::info("ModelThreadPair {}-{}: input loop exiting", driver_ctx_id, model_id);
            break;
        }

        // if we got here, both threads are ready to run
        dlock.unlock();

        // now we officialy enter the input loop
        enter_in_flag = true;
        s_enter_in.notify_all();

        // now continuously pull inputs from the ifmap queues
        uint64_t frame = 0;

        ModelContext* mctx = d->get_mctx(model_id);

        IomapItem* item;
        port_infos_t* p = d->info->port_info[model_id];

        memx_status status = MEMX_STATUS_OTHERS;

        // no timeouts! only frames!
        if(time_limit == 0 && frame_limit > 0) {

            spdlog::debug("ModelThreadPair {}-{}: input stream loop with frame limit {}", driver_ctx_id, model_id, frame_limit);

            while(frame < frame_limit && a_stop_in.load(std::memory_order_consume) == SF_RUN) {

                TSAN_ACQUIRE(&a_stop_in);

                // check stop on empty condition
                if(stop_on_empty) {
                    if(mctx->ifmap_queue->size() == 0) {
                        // queue is empty, so break
                        spdlog::debug("ModelThreadPair {}-{}: input loop hit stop_on_empty", driver_ctx_id, model_id);
                        break;
                    }
                }

                if(mctx->ifmap_queue->pop_timeout_with_ctxpush(item, 500, driver_ctx_id) == false) {
                    // timeout -- break this loop
                    spdlog::debug("ModelThreadPair {}-{}: input loop timed out", driver_ctx_id, model_id);
                    break;
                }

                // HINT: ref_count++ happens in server.cpp!

                for(uint32_t i = 0; i < item->num_fmaps; i++) {
                    // syntax is [driver_ctx_id, port_id, data, timeout]
                    status = memx_stream_ifmap(driver_ctx_id, i + (p->istart_idx), (item->data)[i], 0);
                    if(UNLIKELY(memx_status_error(status))) {
                        break;
                    }
                }
                if(UNLIKELY(memx_status_error(status))) {
                    // error, so break
                    spdlog::critical("ModelThreadPair {}-{}: memx_stream_ifmap() failed with error {}", driver_ctx_id, model_id, (uint32_t) status);
                    item->dest_client->ref_count--;
                    TSAN_ACQUIRE(&item->dest_client->ref_count);
                    TSAN_RELEASE(&item->dest_client->ref_count);
                    break;
                }

                // push this frame's destination to the inflight tracker
                inflights->push(item->dest_client);

                // return this IomapItem to the ifmap freelist
                mctx->ifmap_freelist->push(item);

                frame++;

                //spdlog::debug("ModelThreadPair {}: input loop processed frame {}", model_id, frame);
            } //end while

        }
        else {

            spdlog::debug("ModelThreadPair {}-{}: input stream loop with frame limit {} and time limit {}",
                         driver_ctx_id, model_id, frame_limit, time_limit);

            if(frame_limit == 0) {
                // if frame limit is 0, we will run indefinitely until stopped

                // set a reasonable timeout if time_limit is also 0
                if(time_limit == 0) {
                    time_limit = 1000; // 1 second
                }

                while(a_stop_in.load(std::memory_order_consume) == SF_RUN) {
                    TSAN_ACQUIRE(&a_stop_in);
                    // check stop on empty condition
                    if(stop_on_empty) {
                        if(mctx->ifmap_queue->size() == 0) {
                            // queue is empty, so break
                            break;
                        }
                    }

                    // pop with timeout
                    if(mctx->ifmap_queue->pop_timeout_with_ctxpush(item, time_limit, driver_ctx_id) == false) {
                        // timeout -- break this loop
                        spdlog::debug("ModelThreadPair {}-{}: input loop timed out", driver_ctx_id, model_id);
                        break;
                    }

                    // HINT: ref_count++ happens in server.cpp!

                    for(uint32_t i = 0; i < item->num_fmaps; i++) {
                        // syntax is [driver_ctx_id, port_id, data, timeout]
                        status = memx_stream_ifmap(driver_ctx_id, i + (p->istart_idx), (item->data)[i], time_limit);
                        if(UNLIKELY(memx_status_error(status))) {
                            break;
                        }
                    }
                    if(UNLIKELY(memx_status_error(status))) {
                        // error, so break
                        spdlog::critical("ModelThreadPair {}-{}: memx_stream_ifmap() failed with error {}", driver_ctx_id, model_id, (uint32_t) status);
                        item->dest_client->ref_count--;
                        TSAN_ACQUIRE(&item->dest_client->ref_count);
                        TSAN_RELEASE(&item->dest_client->ref_count);
                        break;
                    }

                    // push this frame's destination to the inflight tracker
                    inflights->push(item->dest_client);

                    // return this IomapItem to the ifmap freelist
                    mctx->ifmap_freelist->push(item);

                    frame++;
                } //end while
            }
            else {
                while(frame < frame_limit && a_stop_in.load(std::memory_order_consume) == SF_RUN) {
                    TSAN_ACQUIRE(&a_stop_in);
                    // check stop on empty condition
                    if(stop_on_empty) {
                        if(mctx->ifmap_queue->size() == 0) {
                            // queue is empty, so break
                            break;
                        }
                    }

                    // pop with timeout
                    if(mctx->ifmap_queue->pop_timeout_with_ctxpush(item, time_limit, driver_ctx_id) == false) {
                        // timeout -- break this loop
                        spdlog::debug("ModelThreadPair {}-{}: input loop timed out", driver_ctx_id, model_id);
                        break;
                    }

                    // HINT: ref_count++ happens in server.cpp!

                    for(uint32_t i = 0; i < item->num_fmaps; i++) {
                        // syntax is [driver_ctx_id, port_id, data, timeout]
                        status = memx_stream_ifmap(driver_ctx_id, i + (p->istart_idx), (item->data)[i], time_limit);
                        if(UNLIKELY(memx_status_error(status))) {
                            break;
                        }
                    }
                    if(UNLIKELY(memx_status_error(status))) {
                        // error, so break
                        spdlog::critical("ModelThreadPair {}-{}: memx_stream_ifmap() failed with error {}", driver_ctx_id, model_id, (uint32_t) status);
                        item->dest_client->ref_count--;
                        TSAN_ACQUIRE(&item->dest_client->ref_count);
                        TSAN_RELEASE(&item->dest_client->ref_count);
                        break;
                    }

                    // push this frame's destination to the inflight tracker
                    inflights->push(item->dest_client);

                    // return this IomapItem to the ifmap freelist
                    mctx->ifmap_freelist->push(item);

                    frame++;
                } //end while
            } //end if time_limit == 0
        } // end if frame_limit == 0

        a_input_done.store(true, std::memory_order_release); // reset input done flag
        inflights->notify(); // wake up the inflight tracker

        spdlog::debug("ModelThreadPair {}-{}: input loop finished readout. Final frame count: {}", driver_ctx_id, model_id, frame);


        // make sure we already officially entered the output loop
        std::unique_lock<std::mutex> enter_lock(m_enter_out);
        s_enter_out.wait(enter_lock, [this] { return enter_out_flag; });
        enter_out_flag = false;

        spdlog::debug("ModelThreadPair {}-{}: input loop make sure we already officially entered the output loop", driver_ctx_id, model_id);

        // if we got here, we either timed out or hit the frame limit
        {
            // notify main thread that input is done
            std::unique_lock<std::mutex> tlock(m_ready);
            num_loop_ready--;
            s_done.notify_all();
            tlock.unlock();
        }

        // check for termination
        {
            std::unique_lock<std::mutex> tlock(m_flags);
            if(a_stop_in.load(std::memory_order_consume) == SF_TERMINATE) { // terminate
                TSAN_ACQUIRE(&a_stop_in);
                tlock.unlock();
                spdlog::info("ModelThreadPair {}-{}: input loop terminating", driver_ctx_id, model_id);
                inflights->notify(); // wake up the inflight tracker
                break;
            }
            tlock.unlock();
        }

        spdlog::debug("ModelThreadPair {}-{}: input loop finished.", driver_ctx_id, model_id);
        inflights->notify(); // wake up the inflight tracker

    }
    // terminated!
}




void ModelThreadPair::output_loop()
{

    IomapItem* dst = nullptr;

    for(;;) {

        // wait for both input and output threads to get ready
        std::unique_lock<std::mutex> dlock(m_ready);
        s_loop_ready.wait(dlock, [this] { return (num_loop_ready == 2) || (a_stop_out.load(std::memory_order_consume) == SF_TERMINATE); });
        TSAN_ACQUIRE(&a_stop_out);
        if(num_loop_ready < 2) {
            // if we got here, it means the program is being shutdown
            // by the main thread somehwere, so we should exit this loop
            dlock.unlock();
            spdlog::info("ModelThreadPair {}-{}: output loop exiting", driver_ctx_id, model_id);
            break;
        }

        // if we got here, both threads are ready to run
        dlock.unlock();

        // now we officialy enter the input loop
        enter_out_flag = true;
        s_enter_out.notify_all();

        port_infos_t* p = d->info->port_info[model_id];

        uint64_t frame = 0;

        // now continuously get outputs from the chip and push to
        // the output queue pointers taken from the inflight tracker
        ContextClient* dest_client;

        spdlog::debug("ModelThreadPair {}-{}: output loop starting readout", driver_ctx_id, model_id);

        memx_status status = MEMX_STATUS_OTHERS;

        // ofmap thread doesn't do timeouts/frame limits, because
        // we don't want to lose any data that the chip has produced
        //          &0x1 = FORCE_HALT or TERMINATE
        while((a_input_done.load(std::memory_order_consume) == false) && (a_stop_out.load(std::memory_order_consume) & 0x1) == SF_RUN) {

            TSAN_ACQUIRE(&a_input_done);
            TSAN_ACQUIRE(&a_stop_out);

            // pop the next destination from the inflight tracker
            if(inflights->pop(dest_client)) {

                //spdlog::debug("ModelThreadPair {} O: popped output queue pair from inflights", model_id);

                // get a free destination from the freelist
                if(dest_client->ofmap_freelists->at(driver_ctx_id)->pop(dst) == false) {
                    // no free destination, so break
                    spdlog::debug("ModelThreadPair {}-{} O: output loop exited before getting a free destination", driver_ctx_id, model_id);
                    dest_client->ref_count--;
                    TSAN_ACQUIRE(&dest_client->ref_count);
                    TSAN_RELEASE(&dest_client->ref_count);
                    break;
                }

                // stream ofmaps from the chip into dst
                for(uint32_t i = 0; i < dst->num_fmaps; i++) {

                    // syntax is [driver_ctx_id, port_id, data, timeout]
                    //spdlog::debug("ModelThreadPair {} O: streaming ofmap frame #{} on port {}", model_id, i, i + (p->ostart_idx));
                    status = memx_stream_ofmap(driver_ctx_id, i + (p->ostart_idx), (dst->data)[i], 0);
                    if(UNLIKELY(memx_status_error(status))) {
                        break;
                    }
                }
                if(UNLIKELY(memx_status_error(status))) {
                    // error, so break
                    spdlog::critical("ModelThreadPair {}-{} O: memx_stream_ofmap() failed with error {}", driver_ctx_id, model_id, (uint32_t) status);
                    dest_client->ref_count--;
                    TSAN_ACQUIRE(&dest_client->ref_count);
                    TSAN_RELEASE(&dest_client->ref_count);
                    break;
                }

                frame++;

                // push the dst to the output queue
                if(dest_client->ofmap_queues->at(driver_ctx_id)->push_timeout(dst, 500) == false) {
                    // timeout -- break this loop
                    spdlog::error("ModelThreadPair {}-{} O: output loop timed out on output queue push", driver_ctx_id, model_id);
                    dest_client->ref_count--;
                    TSAN_ACQUIRE(&dest_client->ref_count);
                    TSAN_RELEASE(&dest_client->ref_count);
                    break;
                }

                dest_client->ref_count--;
                TSAN_ACQUIRE(&dest_client->ref_count);
                TSAN_RELEASE(&dest_client->ref_count);
            }
            else {
                // no more inflight queues, so break
                spdlog::debug("ModelThreadPair {}-{} O: output loop exiting due to being halted by a_input_done", driver_ctx_id, model_id);
                break;
            }
        }

        // do drain_pop on the inflights queue to clear any remaining items
        int drain_count = 0;
        while(inflights->drain_pop(dest_client)) {
            // get a free destination from the freelist
            if(dest_client->ofmap_freelists->at(driver_ctx_id)->pop(dst) == false) {
                // no free destination, so break
                spdlog::warn("ModelThreadPair {}-{} O: drain pop cannot get a free destination -- need to use fmap dumpster and drop data!", driver_ctx_id, model_id);
                std::unique_lock<std::mutex> dumplock(*m_dumpster_lock);
                for(uint32_t i = 0; i < dst->num_fmaps; i++) {
                    // use the dumpster to fill the data
                    //spdlog::debug("ModelThreadPair {} O: using fmap dumpster for ofmap frame #{} on port {}", model_id, i, i + (p->ostart_idx));
                    status = memx_stream_ofmap(driver_ctx_id, i + (p->ostart_idx), dumpster, 0);
                    if(UNLIKELY(memx_status_error(status))) {
                        break;
                    }
                }
                if(UNLIKELY(memx_status_error(status))) {
                    // error, so break
                    spdlog::critical("ModelThreadPair {}-{} O: memx_stream_ofmap() failed with error {}", driver_ctx_id, model_id, (uint32_t) status);
                    dumplock.unlock();
                    dest_client->ref_count--;
                    TSAN_ACQUIRE(&dest_client->ref_count);
                    TSAN_RELEASE(&dest_client->ref_count);
                    break;
                }
                drain_count++;
                dumplock.unlock();
            }
            else {

                for(uint32_t i = 0; i < dst->num_fmaps; i++) {
                    //spdlog::debug("ModelThreadPair {} O: draining ofmap frame #{} on port {}", model_id, i, i + (p->ostart_idx));
                    status = memx_stream_ofmap(driver_ctx_id, i + (p->ostart_idx), (dst->data)[i], 0);
                    if(UNLIKELY(memx_status_error(status))) {
                        break;
                    }
                }
                if(UNLIKELY(memx_status_error(status))) {
                    // error, so break
                    spdlog::error("ModelThreadPair {}-{} O: memx_stream_ofmap() failed with error {}", driver_ctx_id, model_id, (uint32_t) status);
                    dest_client->ref_count--; // decrement the ref count
                    TSAN_ACQUIRE(&dest_client->ref_count);
                    TSAN_RELEASE(&dest_client->ref_count);
                    break;
                }
                drain_count++;

                // push the dst to the output queue
                if(dest_client->ofmap_queues->at(driver_ctx_id)->push_timeout(dst, 500) == false) {
                    spdlog::warn("ModelThreadPair {}-{} O: drain oqueue push timed out -- dropping data", driver_ctx_id, model_id);
                }
            }
            dest_client->ref_count--; // decrement the ref count
            TSAN_ACQUIRE(&dest_client->ref_count);
            TSAN_RELEASE(&dest_client->ref_count);
        }
        spdlog::debug("ModelThreadPair {}-{} O: drained {} items from inflights queue", driver_ctx_id, model_id, drain_count);
        frame += drain_count;

        spdlog::debug("ModelThreadPair {}-{} O: output loop finished readout. Final frame count: {}", driver_ctx_id, model_id, frame);

        // make sure we already officially entered the input loop
        std::unique_lock<std::mutex> enter_lock(m_enter_in);
        s_enter_in.wait(enter_lock, [this] { return enter_in_flag; });
        enter_in_flag = false;

        spdlog::debug("ModelThreadPair {}-{}: output loop make sure we already officially entered the input loop", driver_ctx_id, model_id);


        {
            // notify main thread that output is done
            std::unique_lock<std::mutex> tlock(m_ready);
            num_loop_ready--;
            s_done.notify_all();
        }

        // first check for termination
        {
            std::unique_lock<std::mutex> tlock(m_flags);
            if(a_stop_out.load(std::memory_order_consume) == SF_TERMINATE) { // terminate
                tlock.unlock();
                spdlog::info("ModelThreadPair {}-{}: output loop terminating", driver_ctx_id, model_id);
                break;
            }
        }

        spdlog::debug("ModelThreadPair {}-{} O: output loop finished.", driver_ctx_id, model_id);

    }

    // terminated!

}
