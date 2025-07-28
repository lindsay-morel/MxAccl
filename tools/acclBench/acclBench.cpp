// Copyright (c) 2025 MemryX
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sstream>
#include <atomic>
#ifdef __linux__
    #include<getopt.h>
#else
    #include "windows/getopt.h"
#endif
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <thread>
#include <iomanip>
#include <numeric>
#include <deque>

#include "memx/accl/MxAccl.h"
#include "memx/accl/MxAcclMT.h"
#include "memx/accl/DeviceManager.h"
#include "memx/accl/client.h"

#define MAX_FPS_OPT 1000
#define IW_OPT 1001
#define OW_OPT 1002
#define ND_REQ 1003
#define MT_MODE 1004
#define MD_IDS 1005
#define DS_AL 1006
#define NO_COPY 1007
#define PWR 1008
#define FRQ 1009
#define FLIM 1010

const char*  default_dfp_path = "model/single_ssd_mobilenet_300_MX3.dfp";
#ifdef __linux__
const char*  default_server_addr = "/run/mxa_manager/";
#else
const char* default_server_addr = "127.0.0.1";
#endif
int frame_count = 1000;

static char* dfp_path = NULL;
static const char* server_addr = NULL;
static bool shared_mode = false;
static bool ignore_server = false;
static int server_port_base = 10000;
static int grp_id = 0;
static int max_fps = 0;

static int frame_limit = -1;

static bool multi_stream_bench = false;
static int num_fmap_convert_threads = 1;
static bool verbose = false;
static bool manual_threading = false;

static bool bench_tool = false;
std::atomic_int ms_done_flag = 0;
std::atomic_bool runflag;
std::atomic_bool hello_flag;
static int num_streams = 1;
int num_models = 0;
char fps_text[64] = "FPS = ";
float fps_number = 0.0;
std::chrono::milliseconds start_ms;
std::chrono::milliseconds temp_start_ms;
// accl object
MX::Runtime::MxAccl* accl = NULL;
MX::Runtime::MxAcclMT* accl_mt = NULL;

int num_input_workers = 0;
int num_output_workers = 0;
int num_devices = 1;

//mutit device support
std::vector<int> device_ids;
bool multi_device_bench = false;
// multistream variables
std::vector<std::chrono::steady_clock::time_point> last_send_time_vector;

std::vector<int> sent_frame_count_vector;
std::vector<int> recv_frame_count_vector;
std::vector<MX::Types::MxModelInfo> model_info_vector;
std::vector<std::vector<float*>> ifmap_vector;
std::vector<std::vector<float*>> ofmap_vector;
std::vector<std::chrono::milliseconds> temp_start_ms_vector;
std::vector<std::atomic<float>*> fps_values;
std::vector<int> fps_avg_counters;

//Power consumption data
static bool get_power_usage = false;
std::vector<float> power_values;

std::vector<float> temp_values;
static std::vector<std::deque<float>> temp_windows;
static std::vector<float> temp_window_sums;

constexpr size_t POWER_WINDOW_SIZE = 30;
static std::vector<std::deque<float>> power_windows;
static std::vector<float> power_window_sums;

int dfp_num_chips = 0;
int recv_all_count = 0;
//Manual threads
std::thread** stream_send_threads;
std::thread** stream_recv_threads;

//Frequency
MX::Types::MxFrequencyOption frequency = MX::Types::MxFrequencyOption::FREQ_USE_CONF;

//signal handler
void signal_handler(int p_signal)
{
    runflag.store(false);
}


static void _error_exit(const char* s)
{
    fprintf(stderr, "%s error\n", s);
    exit(EXIT_FAILURE);
}

static void print_usage(char* argv[])
{
    std::cout << "Usage: " << argv[0] << " [options] \n\n" <<
              "Options:\n" <<
              "-h | --help            Print this message\n" <<
              "-H | --hello           Check connection to MXA devices and get device info\n" <<
              "-d | --dfp filename    DFP model file to test, such as '" << default_dfp_path << "'\n" <<
              "-m | --multistream     Run accl bench for multistream\n" <<
              "-n | --numstreams      Number of streams to run multistream accl bench, default= " << num_streams <<
              " for singlestream 2 if multistream is chosen\n"
              "-c | --convert_threads Number of feature map format conversion threads, default= " << num_fmap_convert_threads << "\n" <<
              "-g | --group           Accerator group ID, default=" << grp_id << "\n" <<
              "-f | --frames          Number of frame for testing inference performance, default=" << frame_count << " secs\n" <<
              "-s | --shared_mode     Use Shared Mode (run DFP on mx_server instead of directly accessing hardware)\n" <<
              "-a | --server_addr     Address to mx_server (can be local or remote), default=" << default_server_addr << "\n" <<
              "-p | --server_port     Base port for mx_server connection, default=" << server_port_base << "\n" <<
              //"-i | --ignore_server  Ignore mx_server and run in local mode, disregarding all locks\n" <<  // HIDDEN
              "-v | --verbose         print all the required logs\n" <<
              "--max_fps              maximum allowed FPS per stream\n" <<
              "--iw                   number of input pre-processing workers per model\n" <<
              "--ow                   number of output post-processing workers per model\n" <<
              "--device_ids           MXA device IDs to be used to run benchmark, used in cases of multi device use cases. Takes in a comma separated list of device IDss\n"
              <<
              "--ls                   Allows lenient setup in multi device use cases, uses available devices in case if some of the passed IDs are not available.\n"
              <<
              "--mt                   Runs benchmark tool with Manual Threading model of c++ API\n" <<
              "--set_freq             Override the frequency of connected MXA devices, options = {200,300,400,450,500,600,700,750,800,850} MHz\n"
              "--frame_limit          Number of frames for SchedulerOptions. Default is -1, which means all frames will be processed.\n"
              " ";
}

void parse_device_ids(const std::string &input)
{
    std::stringstream ss(input);
    std::string token;
    while (std::getline(ss, token, ',')) {
        try {
            device_ids.push_back(std::stoi(token));
        }
        catch (const std::invalid_argument &e) {
            std::cerr << "Invalid device ID: " << token << ". Skipping..." << std::endl;
        }
    }
}


static const char short_options[] = "d:Hhmvbn:c:g:f:sa:p:i";

static const struct option
    long_options[] = {
    {"dfp", required_argument, NULL, 'd'},
    {"hello", no_argument, NULL, 'H'},
    {"help", no_argument, NULL, 'h'},
    {"multistream", no_argument, NULL, 'm'},
    {"verbose", no_argument, NULL, 'v'},
    {"bench", no_argument, 0, 'b'},
    {"numstreams", required_argument, NULL, 'n'},
    {"convert_threads", required_argument, NULL, 'c'},
    {"groupID", required_argument, NULL, 'g'},
    {"frames", required_argument, NULL, 'f'},
    {"shared_mode", no_argument, NULL, 's'},
    {"server_addr", required_argument, NULL, 'a'},
    {"server_port", required_argument, NULL, 'p'},
    {"ignore_server", no_argument, NULL, 'i'},
    {"max_fps", required_argument, 0, MAX_FPS_OPT},
    {"iw", required_argument, 0, IW_OPT},
    {"ow", required_argument, 0, OW_OPT},
    {"mt", no_argument, NULL, MT_MODE},
    {"device_ids", required_argument, 0, MD_IDS},
    {"ls", no_argument, NULL, DS_AL},
    {"power", no_argument, NULL, PWR},
    {"set_freq", required_argument, 0, FRQ},
    {"frame_limit", required_argument, 0, FLIM},

    {0, 0, 0, 0 }
};


void print_model_info(MX::Types::MxModelInfo pmodel_info)
{
    std::cout << "\033[3;33m*************************************************\n";
    std::cout << "*               Model Information               *\n";
    std::cout << "*************************************************\033[m\n";
    std::cout << "\n         Model Index : " << pmodel_info.model_index << "        \n";
    std::cout << "\nNum of in featureMaps : " << pmodel_info.num_in_featuremaps << "\n";

    std::cout << "\nIn featureMap Shapes \n";
    for(int i = 0; i < pmodel_info.num_in_featuremaps ; ++i) {
        std::cout << "Shape of featureMap : " << i + 1 << "\n";
        std::cout << "Layer Name : " << pmodel_info.input_layer_names[i] << "\n";
        std::cout << "H = " << pmodel_info.in_featuremap_shapes[i][0] << "\n";
        std::cout << "W = " << pmodel_info.in_featuremap_shapes[i][1] << "\n";
        std::cout << "Z = " << pmodel_info.in_featuremap_shapes[i][2] << "\n";
        std::cout << "C = " << pmodel_info.in_featuremap_shapes[i][3] << "\n";
    }

    std::cout << "\n\nNum of out featureMaps : " << pmodel_info.num_out_featuremaps << "\n";
    std::cout << "\nOut featureMap Shapes \n";
    for(int i = 0; i < pmodel_info.num_out_featuremaps ; ++i) {
        std::cout << "Shape of featureMap : " << i + 1 << "\n";
        std::cout << "Layer Name : " << pmodel_info.output_layer_names[i] << "\n";
        std::cout << "H = " << pmodel_info.out_featuremap_shapes[i][0] << "\n";
        std::cout << "W = " << pmodel_info.out_featuremap_shapes[i][1] << "\n";
        std::cout << "Z = " << pmodel_info.out_featuremap_shapes[i][2] << "\n";
        std::cout << "C = " << pmodel_info.out_featuremap_shapes[i][3] << "\n";
    }

    std::cout << "\033[3;33m*************************************************\033[m\n";
}

MX::Types::MxFrequencyOption get_frequency_option_from_int(int input)
{
    switch (input) {
        case 200: return MX::Types::MxFrequencyOption::FREQ_200MHz;
        case 300: return MX::Types::MxFrequencyOption::FREQ_300MHz;
        case 400: return MX::Types::MxFrequencyOption::FREQ_400MHz;
        case 450: return MX::Types::MxFrequencyOption::FREQ_450MHz;
        case 500: return MX::Types::MxFrequencyOption::FREQ_500MHz;
        case 600: return MX::Types::MxFrequencyOption::FREQ_600MHz;
        case 700: return MX::Types::MxFrequencyOption::FREQ_700MHz;
        case 750: return MX::Types::MxFrequencyOption::FREQ_750MHz;
        case 800: return MX::Types::MxFrequencyOption::FREQ_800MHz;
        case 850: return MX::Types::MxFrequencyOption::FREQ_850MHz;
        default:
            throw std::invalid_argument("Invalid frequency option! Enter a value from options");
    }
}


void generate_input_data(MX::Types::MxModelInfo pmodel_info, std::vector<float*> &pinput_data)
{

    pinput_data.reserve(pmodel_info.num_in_featuremaps);
    // std::cout<<"generating data\n";
    srand((unsigned)time(0));
    for(int i = 0; i < pmodel_info.num_in_featuremaps; i++) {
        float* ifmap = new float[pmodel_info.in_featuremap_sizes[i]];
        for(size_t j = 0; j < pmodel_info.in_featuremap_sizes[i]; j++) {
            ifmap[j] = rand() % 256;
        }
        pinput_data.push_back(ifmap);
    }

}

void cleanup()
{

    for(auto ind : ifmap_vector) {
        for (auto &ifmap : ind) {
            if(ifmap != NULL) {
                delete[] ifmap;
                ifmap = NULL;
            }
        }
    }
    ifmap_vector.clear();

    for(auto ofd : ofmap_vector) {
        for (auto &ofmap : ofd) {
            if(ofmap != NULL) {
                delete[] ofmap;
                ofmap = NULL;
            }
        }
    }
    ofmap_vector.clear();

    if(manual_threading) {
        for(int i = 0; i < (num_models * num_streams); ++i) {
            if(stream_send_threads[i]->joinable()) {
                stream_send_threads[i]->join();
            }

            delete stream_send_threads[i];
            stream_send_threads[i] = NULL;

            stream_recv_threads[i]->join();
            delete stream_recv_threads[i];
            stream_recv_threads[i] = NULL;
        }
        delete[] stream_send_threads;
        stream_send_threads = NULL;
        delete[] stream_recv_threads;
        stream_recv_threads = NULL;
    }
}

/**
 * @brief Maintain a fixed-size moving average of power per device, freezing updates in the final frames.
 *
 * Uses a deque of up to POWER_WINDOW_SIZE samples and a running sum for O(1) average updates.
 * Once recv_frame_count_vector[i] exceeds frame_count - window.size(), the window no longer changes.
 */
void get_power_statistics()
{

    // for each device, get the power consumption and put it in a
    // vector called avg

    // local mode does averaging here in acclBench
    if(!shared_mode){
        std::vector<float> avg(num_devices);

        for(int i = 0; i < num_devices; ++i) {
            int devid = device_ids[i];
            if(manual_threading) {
                if(accl_mt->can_get_power_consumption(devid)) {
                    avg[i] = accl_mt->get_power(devid);
                }
                else {
                    avg[i] = 0;
                }
            }
            else {
                if(accl->can_get_power_consumption(devid)) {
                    avg[i] = accl->get_power(devid);
                }
                else {
                    avg[i] = 0;
                }
            }
        }

        for (int i = 0; i < num_devices; ++i) {
            auto &window = power_windows[i];
            auto &sum    = power_window_sums[i];
            int  recv    = recv_frame_count_vector[i];
            size_t dq    = window.size();

            // Freeze once we're within the last 'dq' frames
            if (dq > 0
                    && frame_count >= 2 * static_cast<int>(dq)
                    && recv > frame_count - static_cast<int>(dq)) {
                power_values[i] = sum / static_cast<float>(dq);
                continue;
            }

            // Sliding‐window update
            float p = avg[i];
            if (p <= 17000) { // Skip invalid power readings. The module has a power limit of 15W.
                window.push_back(p);
                sum += p;
            }

            if (window.size() > POWER_WINDOW_SIZE) {
                sum -= window.front();
                window.pop_front();
            }

            power_values[i] = sum / static_cast<float>(window.size());
        }
    }
    else {
        // In shared mode, we just get the avg power from the server
        for(int i = 0; i < num_devices; ++i) {
            int devid = device_ids[i];
            if(manual_threading) {
                if(accl_mt->can_get_power_consumption(devid)) {
                    power_values[i] = accl_mt->get_power(devid);
                }
                else {
                    power_values[i] = 0;
                }
            }
            else {
                if(accl->can_get_power_consumption(devid)) {
                    power_values[i] = accl->get_power(devid);
                }
                else {
                    power_values[i] = 0;
                }
            }
        }
    }
}


void get_temp_statistics()
{

    // Like power stats above, but we use get_max_temperature instead of get_power
    // and we don't need to check if the device is able to get temp info
    // We use the same window size as power.

    // local mode does averaging here in acclBench
    if(!shared_mode){
        std::vector<float> avg(num_devices);

        for(int i = 0; i < num_devices; ++i) {
            int devid = device_ids[i];
            if(manual_threading) {
                avg[i] = accl_mt->get_max_temperature(devid);
            }
            else {
                avg[i] = accl->get_max_temperature(devid);
            }
        }

        for (int i = 0; i < num_devices; ++i) {
            auto &window = temp_windows[i];
            auto &sum    = temp_window_sums[i];
            int  recv    = recv_frame_count_vector[i];
            size_t dq    = window.size();

            // Freeze once we're within the last 'dq' frames
            if (dq > 0
                    && frame_count >= 2 * static_cast<int>(dq)
                    && recv > frame_count - static_cast<int>(dq)) {
                temp_values[i] = sum / static_cast<float>(dq);
                continue;
            }

            // Sliding‐window update
            float p = avg[i];
            window.push_back(p);
            sum += p;

            if (window.size() > POWER_WINDOW_SIZE) {
                sum -= window.front();
                window.pop_front();
            }

            temp_values[i] = sum / static_cast<float>(window.size());
        }
    }
    else {
        // In shared mode, we just get the temperature from the server
        for (int i = 0; i < num_devices; ++i) {
            int devid = device_ids[i];
            if (manual_threading) {
                temp_values[i] = accl_mt->get_max_temperature(devid);
            }
            else {
                temp_values[i] = accl->get_max_temperature(devid);
            }
        }
    }
}


bool incallback_ms(vector<const MX::Types::FeatureMap*> dst, int streamLabel)
{
    if (max_fps > 0) {
        using clk = std::chrono::steady_clock;
        auto interval = std::chrono::microseconds(1'000'000 / max_fps);
        auto now = clk::now();
        auto next_allowed = last_send_time_vector[streamLabel] + interval;

        if (now < next_allowed) {
            std::this_thread::sleep_until(next_allowed);
            now = clk::now();
        }
        last_send_time_vector[streamLabel] = now;
    }

    if (sent_frame_count_vector[streamLabel] < frame_count && runflag.load()) {
        for (int i = 0; i < model_info_vector[streamLabel].num_in_featuremaps; ++i) {
            //printf(">Sending data to stream %d, frame %d\n", streamLabel, sent_frame_count_vector[streamLabel]);
            dst[i]->set_data(ifmap_vector[streamLabel][i]);
            //printf("-SENT data to stream %d, frame %d\n", streamLabel, sent_frame_count_vector[streamLabel]);

        }
        sent_frame_count_vector[streamLabel]++;
        return true;
    }
    else {
        ms_done_flag++;
        TSAN_ACQUIRE(&ms_done_flag);
        TSAN_RELEASE(&ms_done_flag);
        return false;
    }
}

bool outcallback_ms(vector<const MX::Types::FeatureMap*> src, int streamLabel)
{
    if(recv_frame_count_vector[streamLabel] < frame_count) {
        // std::cout<<"outcallback called \n";
        for(int i = 0; i < model_info_vector[streamLabel].num_out_featuremaps; ++i) {
            //printf(">Receiving data from stream %d, frame %d\n", streamLabel, recv_frame_count_vector[streamLabel]);
            src[i]->get_data(ofmap_vector[streamLabel][i]);
            //printf("-RECV'd data from stream %d, frame %d\n", streamLabel, recv_frame_count_vector[streamLabel]);
        }


        if( recv_frame_count_vector[streamLabel] != 0 && recv_frame_count_vector[streamLabel] % 50 == 0) {
            std::chrono::milliseconds duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) -
                temp_start_ms_vector[streamLabel];

            float fps = (float) 50 * 1000 / (float)(duration.count());
            fps_avg_counters[streamLabel]++;
            float cur = fps_values[streamLabel]->load(std::memory_order_relaxed);
            TSAN_ACQUIRE(fps_values[streamLabel]);
            fps_values[streamLabel]->store(((cur * (fps_avg_counters[streamLabel] - 1)) + fps) / fps_avg_counters[streamLabel],
                                           std::memory_order_relaxed);
            TSAN_RELEASE(fps_values[streamLabel]);
            temp_start_ms_vector[streamLabel] = std::chrono::duration_cast<std::chrono::milliseconds>
                                                (std::chrono::system_clock::now().time_since_epoch());
        }
        recv_frame_count_vector[streamLabel]++;
        return true;
    }
    else {

        return false;
    }

}

void print_bench_setting_info()
{

    std::cout << "\nNumber of models in DFP               = " << num_models << "\n";
    std::cout << "Number of streams                     = " << num_streams << "\n";
    std::cout << "Number of streams to be connected     = " << num_streams* num_models << "\n";
    std::cout << "Number of frame per stream            = " << frame_count << "\n";
    std::cout << "Number of input workers set to        = " << ((num_input_workers == 0
              || num_input_workers > num_streams) ? num_streams : num_input_workers) << "\n";
    std::cout << "Number of output workers set to       = " << ((num_output_workers == 0
              || num_output_workers > num_streams) ? num_streams : num_output_workers) << "\n";
    std::cout << "Number of devices used                = " << num_devices << "\n";
    std::cout << "Number of FMap conversion threads     = " << num_fmap_convert_threads << "\n";
    std::cout << "mx_server connection                  = " << server_addr << ":" << server_port_base << "\n";
    if(shared_mode) {
        std::cout << "Shared mode                           = ON\n";
    }
    if(ignore_server) {
        std::cout << "Ignoring mx_server connection         = ON\n";
    }

}

void display_fps()
{

    // Print table header
    std::cout << std::setw(10) << "Model" << std::setw(10) << "Stream" << std::setw(15) << "FPS" << std::endl;
    std::cout << std::setw(35) << std::setfill('-') << "-" << std::endl;
    std::cout << std::setfill(' ');

    // Print values
    for (size_t i = 0; i < fps_values.size(); ++i) {
        std::cout << std::setw(10) << model_info_vector[i].model_index
                  << std::setw(10) << i << std::setw(15) << std::fixed << std::setprecision(1)
                  << fps_values[i]->load(std::memory_order_relaxed) << std::endl;
        TSAN_ACQUIRE(fps_values[i]); // Ensure memory visibility for TSAN
    }
    std::cout << std::endl; // Space between tables

    // Print Temp Statistics if verbose
    get_temp_statistics();
    std::cout << std::setw(10) << "Device" << std::setw(15) << "Temp (C)" << std::endl;
    std::cout << std::setw(25) << std::setfill('-') << "-" << std::endl;
    std::cout << std::setfill(' ');

    // Print Power values
    for (size_t i = 0; i < temp_values.size(); ++i) {
        std::cout << std::setw(10) << int(device_ids[i])
                  << std::setw(15) << std::fixed << std::setprecision(1)
                  << temp_values[i] << std::endl;
    }
}

void display_fps_and_power()
{
    // Print FPS Table Header
    get_power_statistics();
    get_temp_statistics();
    std::cout << std::setw(10) << "Model" << std::setw(10) << "Stream" << std::setw(15) << "FPS" << std::endl;
    std::cout << std::setw(35) << std::setfill('-') << "-" << std::endl;
    std::cout << std::setfill(' ');

    // Print FPS values
    for (size_t i = 0; i < fps_values.size(); ++i) {
        std::cout << std::setw(10) << model_info_vector[i].model_index
                  << std::setw(10) << i
                  << std::setw(15) << std::fixed << std::setprecision(1)
                  << fps_values[i]->load(std::memory_order_relaxed) << std::endl;
        TSAN_ACQUIRE(fps_values[i]);
    }

    std::cout << std::endl; // Space between tables

    // Print Power Statistics Table Header
    std::cout << std::setw(10) << "Device" << std::setw(15) << "Power (W)" << std::setw(15) << "Temp (C)" << std::endl;
    std::cout << std::setw(40) << std::setfill('-') << "-" << std::endl;
    std::cout << std::setfill(' ');

    // Print Power values
    for (size_t i = 0; i < power_values.size(); ++i) {
        std::cout << std::setw(10) << int(device_ids[i])
                  << std::setw(15) << std::fixed << std::setprecision(1)
                  << power_values[i] / 1000
                  << std::setw(15) << std::fixed << std::setprecision(1)
                  << temp_values[i] << std::endl;

    }

    // Move cursor up to refresh display
    // std::cout << std::flush;
    // std::cout << "\033[" << (connected_streams + power_values.size() + 4) << "A";
    // std::this_thread::sleep_for(std::chrono::seconds(1));
}


void model_bench(int num_models)
{

    int model_unique_stream_start = 0;
    start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    fps_values.reserve(num_models * num_streams);
    fps_avg_counters.reserve(num_models * num_streams);
    ifmap_vector.reserve(num_models * num_streams);
    ofmap_vector.reserve(num_models * num_streams);
    sent_frame_count_vector.reserve(num_models * num_streams);
    recv_frame_count_vector.reserve(num_models * num_streams);
    temp_start_ms_vector.reserve(num_models * num_streams);
    model_info_vector.reserve(num_models * num_streams);
    last_send_time_vector.reserve(num_models * num_streams);

    for(int model_index = 0; model_index < num_models ; model_index++) {
        MX::Types::MxModelInfo minfo = accl->get_model_info(model_index);
        if(verbose) { print_model_info(minfo); }

        for(int stream_id = 0; stream_id < num_streams ; stream_id++) {

            temp_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

            model_info_vector.push_back(minfo);


            // model_info = minfo;
            //input config
            std::vector<float*> in_data;
            generate_input_data(minfo, in_data);
            ifmap_vector.push_back(in_data);

            //output config
            std::vector<float*> out_data;
            out_data.reserve(minfo.num_out_featuremaps);
            for(int i = 0; i < minfo.num_out_featuremaps ; i++) {
                float* ofmap = new float[minfo.out_featuremap_sizes[i]];
                // std::cout<<"size of featuremap " << i << "  " << minfo.out_featuremap_sizes[i] << "\n";
                out_data.push_back(ofmap);
            }

            ofmap_vector.push_back(out_data);

            sent_frame_count_vector.push_back(0);
            recv_frame_count_vector.push_back(0);
            if (max_fps > 0) {
                auto interval = std::chrono::microseconds(1'000'000 / max_fps);
                // set every stream’s last_send_time so the first frame goes out immediately
                last_send_time_vector.push_back(std::chrono::steady_clock::now() - interval);
            }
            temp_start_ms_vector.push_back(temp_start_ms);
            fps_values.push_back(new std::atomic<float>(0.0));
            fps_avg_counters.push_back(0);
            // running_fps_values.push_back(0.0);
            accl->connect_stream(&incallback_ms, &outcallback_ms, model_unique_stream_start + stream_id, model_index );
            if(verbose) {
                std::cout << "Connected stream " << model_unique_stream_start + stream_id << " for model " << model_index << "\n\n";
                std::cout << "\033[3;33m*************************************************\033[m\n";
            }
        }
        model_unique_stream_start += num_streams;
    }
    int connected_streams = accl->get_num_streams();
    accl->start();
    std::cout << "Number of chips the dfp is compiled for = " << dfp_num_chips << "\n\n";
    if(verbose) {
        std::cout << "\n\n\n" << std::flush;

        std::cout << "*************************************************\033[m\n\n";
    }
    if(!bench_tool) {
        // wait a little before starting to check for temp/power/fps info
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        while(ms_done_flag.load(std::memory_order_consume) < connected_streams) {
            TSAN_ACQUIRE(&ms_done_flag);
            if(!get_power_usage) {
                display_fps();
                std::cout << "\033[" << ((num_models*connected_streams) + temp_values.size() + 5) << "A";
            }
            else {
                display_fps_and_power();
                std::cout << "\033[" << ((num_models*connected_streams) + power_values.size() + 5) << "A";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
    }

    accl->wait();

    if(!get_power_usage) {
        display_fps();
    }
    else {
        display_fps_and_power();
    }


    accl->stop();
    std::chrono::milliseconds duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) - start_ms;
    float fps_total = 0.0;
    for(auto fps : fps_values) {
        fps_total += fps->load(std::memory_order_relaxed);
        TSAN_ACQUIRE(fps);
    }
    float fps_per_stream = fps_total / connected_streams;
    std::cout << "\rAverage FPS per stream : " << fps_per_stream << "\033[m\n";
    std::cout << "\rAverage FPS for DFP    : " << fps_total << "\033[m\n";
    if (get_power_usage) {
        // sum up the last-window power readings (in mW), convert to W
        float total_mW = std::accumulate(power_values.begin(), power_values.end(), 0.0f);
        float total_W  = total_mW / 1000.0f;
        std::cout << "\rPower for DFP          : " << std::fixed << std::setprecision(1)
                  << total_W << " W\n";
    }
    if(verbose) {
        std::cout << "\n\n*************************************************\033[m\n";
        std::cout << "\n\n";
    }
    cleanup();
}

void send_data(int stream_label)
{
    while(sent_frame_count_vector[stream_label]  < frame_count && runflag.load()) {
        if (max_fps > 0) {
            using clk = std::chrono::steady_clock;
            auto interval = std::chrono::microseconds(1'000'000 / max_fps);
            auto now = clk::now();
            auto next_allowed = last_send_time_vector[stream_label] + interval;

            if (now < next_allowed) {
                std::this_thread::sleep_until(next_allowed);
                now = clk::now();
            }
            last_send_time_vector[stream_label] = now;
        }
        accl_mt->send_input(ifmap_vector[stream_label], model_info_vector[stream_label].model_index, stream_label, false);
        sent_frame_count_vector[stream_label]++;
    }
}

void receive_data(int model_index, int stream_id_recv)
{
    MX::Types::MxModelInfo minfo = accl_mt->get_model_info(model_index);
    std::vector<float*> out_data;
    out_data.reserve(minfo.num_out_featuremaps);
    for(int i = 0; i < minfo.num_out_featuremaps ; i++) {
        float* ofmap = new float[minfo.out_featuremap_sizes[i]];
        out_data.push_back(ofmap);
    }
    float fps = 0.0;
    while(recv_frame_count_vector[stream_id_recv]  < frame_count && runflag.load()) {
        if(sent_frame_count_vector[stream_id_recv] <= recv_frame_count_vector[stream_id_recv]) {
            continue;
        }
        accl_mt->receive_output(out_data, model_index, stream_id_recv, false);

        if( recv_frame_count_vector[stream_id_recv] != 0 && recv_frame_count_vector[stream_id_recv] % 50 == 0) {
            std::chrono::milliseconds duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) -
                temp_start_ms_vector[stream_id_recv];

            fps = (float) 50 * 1000 / (float)(duration.count());
            fps_avg_counters[stream_id_recv]++;
            float cur = fps_values[stream_id_recv]->load(std::memory_order_relaxed);
            TSAN_ACQUIRE(fps_values[stream_id_recv]);
            fps_values[stream_id_recv]->store(((cur * (fps_avg_counters[stream_id_recv] - 1)) + fps) / fps_avg_counters[stream_id_recv],
                                              std::memory_order_relaxed);
            TSAN_RELEASE(fps_values[stream_id_recv]);
            temp_start_ms_vector[stream_id_recv] = std::chrono::duration_cast<std::chrono::milliseconds>
                                                   (std::chrono::system_clock::now().time_since_epoch());

        }
        recv_frame_count_vector[stream_id_recv]++;
    }
    for(auto &fmap : out_data) {
        delete [] fmap;
        fmap = NULL;
    }
}

void manual_model_bench(int num_models)
{

    int model_unique_stream_start = 0;
    start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    fps_values.reserve(num_models * num_streams);
    fps_avg_counters.reserve(num_models * num_streams);
    ifmap_vector.reserve(num_models * num_streams);
    ofmap_vector.reserve(num_models * num_streams);
    sent_frame_count_vector.reserve(num_models * num_streams);
    recv_frame_count_vector.reserve(num_models * num_streams);
    temp_start_ms_vector.reserve(num_models * num_streams);
    model_info_vector.reserve(num_models * num_streams);
    last_send_time_vector.reserve(num_models * num_streams);
    stream_recv_threads = new std::thread *[num_models * num_streams];
    stream_send_threads = new std::thread *[num_models * num_streams];
    for(int model_index = 0; model_index < num_models ; model_index++) {
        MX::Types::MxModelInfo minfo = accl_mt->get_model_info(model_index);
        if(verbose) {
            print_model_info(minfo);
        }

        for(int stream_id = 0; stream_id < num_streams ; stream_id++) {

            temp_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

            model_info_vector.push_back(minfo);

            std::vector<float*> in_data;
            generate_input_data(minfo, in_data);
            ifmap_vector.push_back(in_data);

            sent_frame_count_vector.push_back(0);
            recv_frame_count_vector.push_back(0);
            temp_start_ms_vector.push_back(temp_start_ms);
            fps_values.push_back(new std::atomic<float>(0.0));
            fps_avg_counters.push_back(0);
            if (max_fps > 0) {
                auto interval = std::chrono::microseconds(1'000'000 / max_fps);
                // set every stream’s last_send_time so the first frame goes out immediately
                last_send_time_vector.push_back(std::chrono::steady_clock::now() - interval);
            }



            stream_send_threads[model_unique_stream_start + stream_id] = new std::thread(send_data, model_unique_stream_start + stream_id);
            stream_recv_threads[model_unique_stream_start + stream_id] = new std::thread(receive_data, model_index,
                    model_unique_stream_start + stream_id );

            if(verbose) {
                std::cout << "Created Thread " << model_unique_stream_start + stream_id << " for model " << model_index << "\n\n";
                std::cout << "\033[3;33m*************************************************\033[m\n";
            }
        }
        model_unique_stream_start += num_streams;
    }
    std::cout << "Number of chips used per device   = " << dfp_num_chips << "\n\n";
    if(verbose) {
        std::cout << "\n\n\n" << std::flush;

        std::cout << "*************************************************\033[m\n\n";
    }

    while(runflag.load()) {
        if(!get_power_usage) {
            display_fps();
            std::cout << "\033[" << ((num_models * num_streams) + temp_values.size() + 5) << "A";
        }
        else {
            display_fps_and_power();
            std::cout << "\033[" << ((num_models * num_streams) + power_values.size() + 5) << "A";
        }
        std::cout << std::flush;

        std::this_thread::sleep_for(std::chrono::seconds(2));
        for(int i = 0; i < (num_models * num_streams); i++) {
            if(recv_frame_count_vector[i] >= frame_count) {
                recv_all_count++;
            }
            else {
                break;
            }
        }
        if(recv_all_count == (num_models * num_streams)) {
            runflag.store(false);
        }
    }

    if(!get_power_usage) {
        display_fps();
    }
    else {
        display_fps_and_power();
    }
    std::cout << "\n";
    std::chrono::milliseconds duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) - start_ms;
    float fps_total = 0.0;
    for(auto fps : fps_values) {
        fps_total += fps->load(std::memory_order_relaxed);
        TSAN_ACQUIRE(fps);
    }
    float fps_per_stream = fps_total / (num_models * num_streams);
    std::cout << "\rAverage FPS per stream : " << fps_per_stream << "\033[m\n";
    std::cout << "\rAverage FPS for DFP    : " << fps_total << "\033[m\n";
    if(verbose) {
        std::cout << "\n\n*************************************************\033[m\n";
        std::cout << "\n\n";
    }
    cleanup();
}

int main(int argc, char** argv)
{

    if (argc == 1) {
        print_usage(argv);
        exit(EXIT_FAILURE);
    }

    // set default
    server_addr = default_server_addr;

    for (;;) {
        int idx;
        int c;

        c = getopt_long(argc, argv,
                        short_options, long_options, &idx);

        if (-1 == c || hello_flag.load()) {
            break;
        }

        switch (c) {
            case 0: /* getopt_long() flag */
                break;
            case 'H':
                hello_flag.store(true);
                if(argc > 2) {
                    std::cout << "Given arguments along with hello option \n";
                    std::cout << "Parameters along with -H or --hello are ignored \n\n";
                }
                break;

            case 'd':
                dfp_path = optarg;
                break;
            
            case 's':
                shared_mode = true;
                break;

            case 'a':
                server_addr = optarg;
                break;

            case 'p':
                errno = 0;
                server_port_base = strtol(optarg, NULL, 0);
                if (errno) {
                    _error_exit(optarg);
                }
                break;

            case 'i':
                ignore_server = true;
                break;

            case 'm':
                multi_stream_bench = true;
                num_streams = 2;
                break;

            case 'v':
                verbose = true;
                break;

            case 'b':
                bench_tool = true;
                break;

            case 'h':
                print_usage(argv);
                exit(EXIT_SUCCESS);
                break;

            case 'n':
                errno = 0;
                num_streams = strtol(optarg, NULL, 0);
                if (errno) {
                    _error_exit(optarg);
                }
                break;

            case 'c':
                errno = 0;
                num_fmap_convert_threads = strtol(optarg, NULL, 0);
                if (errno) {
                    _error_exit(optarg);
                }
                break;

            case 'g':
                errno = 0;
                grp_id = strtol(optarg, NULL, 0);

                if (errno) {
                    _error_exit(optarg);
                }
                break;

            case 'f':
                errno = 0;
                frame_count = strtol(optarg, NULL, 0);
                if (errno) {
                    _error_exit(optarg);
                }
                break;

            case MAX_FPS_OPT:
                errno = 0;
                max_fps = strtol(optarg, NULL, 0);
                if (errno) {
                    _error_exit(optarg);
                }
                break;

            case IW_OPT:
                errno = 0;
                num_input_workers = strtol(optarg, NULL, 0);
                if (errno) {
                    _error_exit(optarg);
                }
                break;

            case OW_OPT:
                errno = 0;
                num_output_workers = strtol(optarg, NULL, 0);
                if (errno) {
                    _error_exit(optarg);
                }
                break;
            case MD_IDS:
                errno = 0;
                if(optarg) {
                    parse_device_ids(optarg);
                }
                if(device_ids.size() > 1) {
                    multi_device_bench = true;
                }
                else {
                    grp_id = device_ids[0];
                }
                if (errno) {
                    _error_exit(optarg);
                }
                break;

            case MT_MODE:
                frame_count++;
                manual_threading = true;
                break;
            case PWR:
                get_power_usage = true;
                break;
            case FRQ:
                errno = 0;
                if(optarg) {
                    try {
                        int freqin = std::stoi(optarg);
                        frequency = get_frequency_option_from_int(freqin);
                    }
                    catch(const std::invalid_argument &e) {
                        print_usage(argv);
                        _error_exit(optarg);
                        std::cerr << e.what() << "\n";
                    }
                }

                break;
            case FLIM:
                errno = 0;
                frame_limit = strtol(optarg, NULL, 0);
                if (errno) {
                    _error_exit(optarg);
                }
                break;

            default:
                print_usage(argv);
                exit(EXIT_FAILURE);
        }
    }

    // local mode must explicitly use a device id
    if(!shared_mode) {
        if(grp_id < 0 && device_ids.size() <= 1) {
            grp_id = 0;
            device_ids.clear();
            device_ids.push_back(0);
        }
    }

    if(device_ids.size() == 0){
        device_ids.push_back(grp_id);
    }

    signal(SIGINT, signal_handler);

    if(hello_flag) {
        runflag.store(false);
        std::cout << "Hello from MXA! \n";
        MX::Runtime::DeviceManager* device_manager = new MX::Runtime::DeviceManager();
        MX::Runtime::Client* client_ = new MX::Runtime::Client();
        if(client_->init_connection(server_addr, server_port_base) == false) {
            delete client_;
            device_manager->discover_devices_direct();
        }
        else{
            device_manager->discover_devices_remote(client_);
            delete client_;
        }
        device_manager->print_devices_info();      
        delete device_manager;
    }
    else {

        MX::RPC::SchedulerOptions opts;
        if(frame_limit <= 0) {
            frame_limit = (frame_count * num_streams) + 10;
        }

        opts.frame_limit = frame_limit;
        opts.time_limit = 0;
        opts.stop_on_empty = false;
        opts.ifmap_queue_size = 16;
        opts.ofmap_queue_size = 12;

        std::cout << "\033[3;34m*************************************************\n";
        std::cout << "*      Evaluate dfp performance using MX3       *\n";
        std::cout << "*************************************************\033[m\n\n";

        // if the pointers aren't equal, user manually set server address
        if( (server_addr != default_server_addr) || (server_port_base != 10000) || (shared_mode == true) ) {
            std::cout << "mxa_manager connection at " << server_addr << ":" << server_port_base << "\n";
            if(shared_mode) {
                std::cout << "Mode: SHARED\n\n";
            }
            else {
                std::cout << "Mode: LOCAL\n\n";
            }
        }

        runflag.store(true);
        if (dfp_path == NULL) {
            std::cout << "please specify the dfp file\n";
            print_usage(argv);
            exit(EXIT_FAILURE);
            return -1;
        }
        if(manual_threading) {
            if(multi_device_bench) {
                accl_mt = new MX::Runtime::MxAcclMT(dfp_path, device_ids, {true,true}, !shared_mode,
                                                    opts, {false, 0}, server_addr, server_port_base, ignore_server);
                //for(size_t i = 0; i < device_ids.size(); i++) {
                //    accl_mt->set_operating_frequency(device_ids[i], frequency);
                //}
                if(accl_mt->is_ready() == false) {
                    std::cout << "\033[1;33mCANNOT CONNECT TO DEVICE\033[0m\n";
                    exit(EXIT_FAILURE);
                    return -1;
                }
            }
            else {
                accl_mt = new MX::Runtime::MxAcclMT(dfp_path, device_ids, {true,true}, !shared_mode,
                                                    opts, {false, 0}, server_addr, server_port_base, ignore_server);
                //for(size_t i = 0; i < device_ids.size(); i++) {
                //    accl_mt->set_operating_frequency(device_ids[i], frequency);
                //}
                if(accl_mt->is_ready() == false) {
                    std::cout << "\033[1;33mCANNOT CONNECT TO DEVICE\033[0m\n";
                    exit(EXIT_FAILURE);
                    return -1;
                }
                device_ids.clear();
                device_ids.push_back(grp_id);
            }
            num_models = accl_mt->get_num_models();
            for(int i = 0; i < num_models; i++) {
                accl_mt->set_parallel_fmap_convert(num_fmap_convert_threads, i);
            }

            dfp_num_chips = accl_mt->get_dfp_num_chips();
            if(verbose) {
                print_bench_setting_info();
            }

            // Power values setup
            if(get_power_usage) {
                if(accl_mt->can_get_power_consumption(0)) {
                    for(size_t i = 0; i <  device_ids.size(); i++) {
                        power_values.push_back(0.00);
                    }
                    power_windows.resize(device_ids.size());
                    power_window_sums.assign(device_ids.size(), 0.0f);
                }
                else {
                    get_power_usage = false;
                    std::cout << "\033[1;33mCANNOT GET POWER CONSUMPTION DATA\033[0m\n";
                }
            }
            temp_windows.resize(device_ids.size());
            temp_window_sums.assign(device_ids.size(), 0.0f);
            for(size_t i = 0; i <  device_ids.size(); i++) {
                temp_values.push_back(0.00);
            }
            manual_model_bench(num_models);

        }
        else {
            if(multi_device_bench) {
                accl = new MX::Runtime::MxAccl(dfp_path, device_ids, {true,true}, !shared_mode,
                                               opts, {false, 0}, server_addr, server_port_base, ignore_server);
                if(device_ids[0] >= 0) {
                    // use the device ids specified by the user
                    //for(size_t i = 0; i < device_ids.size(); i++) {
                    //    accl->set_operating_frequency(device_ids[i], frequency);
                    //}
                }
                if(accl->is_ready() == false) {
                    std::cout << "\033[1;33mCANNOT CONNECT TO DEVICE\033[0m\n";
                    exit(EXIT_FAILURE);
                    return -1;
                }
            }
            else {
                accl = new MX::Runtime::MxAccl(dfp_path, device_ids, {true,true}, !shared_mode,
                                               opts, {false, 0}, server_addr, server_port_base, ignore_server);
                if(device_ids[0] >= 0) {
                    // use the device ids specified by the user
                    //for(size_t i = 0; i < device_ids.size(); i++) {
                    //    accl->set_operating_frequency(device_ids[i], frequency);
                    //}
                }
                if(accl->is_ready() == false) {
                    std::cout << "\033[1;33mCANNOT CONNECT TO DEVICE\033[0m\n";
                    exit(EXIT_FAILURE);
                    return -1;
                }
                device_ids.clear();
                device_ids.push_back(grp_id);
            }

            num_devices = device_ids.size();

            accl->set_num_workers(num_input_workers, num_output_workers);
            num_models = accl->get_num_models();
            for(int i = 0; i < num_models; i++) {
                accl->set_parallel_fmap_convert(num_fmap_convert_threads, i);
            }
            dfp_num_chips = accl->get_dfp_num_chips();
            if(verbose) {
                print_bench_setting_info();
            }

            // Power values set  up
            if(get_power_usage) {
                if(accl->can_get_power_consumption(0)) {
                    for(size_t i = 0; i <  device_ids.size(); i++) {
                        power_values.push_back(0.00);
                    }
                    power_windows.resize(device_ids.size());
                    power_window_sums.assign(device_ids.size(), 0.0f);
                }
                else {
                    get_power_usage = false;
                    std::cout << "\033[1;33mCANNOT GET POWER CONSUMPTION DATA\033[0m\n";
                }
            }

            temp_windows.resize(device_ids.size());
            temp_window_sums.assign(device_ids.size(), 0.0f);
            for(size_t i = 0; i <  device_ids.size(); i++) {
                temp_values.push_back(0.00);
            }


            model_bench(num_models);
        }


        // clean up fps_values
        for(auto &fps : fps_values) {
            delete fps;
            fps = NULL;
        }

        if(accl != NULL) {
            delete accl;
            accl = NULL;
        }
        else {
            accl = NULL;
        }
        if(accl_mt != NULL) {
            delete accl_mt;
            accl_mt = NULL;
        }
        else {
            accl_mt = NULL;
        }

        if(!bench_tool) {
            std::cout << "\n\n\033[3;33m Bench for " << num_models << " Model(s) Done \n";
        }
    }
    //         std::cout << "\033[3;33m*************************************************\n\n\033[m";
    //         std::cout<<"Wait, we are not done yet \n ";
    //         std::cout<<"Calculating latency...\n";
    // }
    return 0;
}
