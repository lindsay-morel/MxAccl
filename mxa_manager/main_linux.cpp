// Copyright (c) 2025 MemryX
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "server.h"
#include "color_print.h"

#include <iostream>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <csignal>
#include <fstream>
#include <filesystem>

#include "spdlog/spdlog.h"
#include "spdlog/cfg/env.h"

#include <memx/accl/utils/cpu_opts.h>

MX::Manager::Server* server;

// ctrl+c handler to stop the server gracefully
void signal_handler(int signum)
{
    spdlog::warn("[Server] Caught signal {}: stopping server...", signum);
    if(server) {
        delete server; // will call Server::kill() internally
        server = nullptr;
        exit(EXIT_SUCCESS);
    }
    else {
        spdlog::error("[Server] Server pointer is null, cannot stop server gracefully");
        exit(EXIT_FAILURE);
    }
}


// function to parse the mxa_manager.conf file from
// a fixed path location: one for Linux and one for Windows
bool parse_config_file(std::string* addr, unsigned short* base_port)
{
    std::string config_path;
#ifdef _WIN32
    config_path = "C:\\Program Files\\memryx\\mxa_manager.conf";
#else
    config_path = "/etc/memryx/mxa_manager.conf";
#endif

    if(!std::filesystem::exists(config_path)) {
        spdlog::critical("Config file not found at {}", config_path);
        return false;
    }
    std::ifstream config_file(config_path);
    if(!config_file.is_open()) {
        spdlog::critical("Unable to open config file at {}", config_path);
        return false;
    }
    std::string line;

    // the syntax of the config file ignores all lines starting with #,
    // then looks for these two variables:
    // LISTEN_ADDRESS="address_as_string"
    // BASE_PORT=port_as_integer
    //
    // addr is then assgined to the address string and base_port to the port integer
    bool found_addr = false;
    bool found_port = false;
    while(std::getline(config_file, line)) {
        if(line[0] == '#') { continue; } // ignore comments
        if(line.find("LISTEN_ADDRESS=") != std::string::npos) {
            *addr = line.substr(16, line.length() - 17);
            found_addr = true;
        }
        else if(line.find("BASE_PORT=") != std::string::npos) {
            *base_port = std::stoi(line.substr(10));
            found_port = true;
        }
    }

    if(!found_addr || !found_port) {
        spdlog::critical("Config file is missing required variables: LISTEN_ADDRESS or BASE_PORT");
        return false;
    }

    return true;
}


// main function parses the config file, creates a Server object,
// then starts the server with .run()
int main()
{
    std::string addr;
    unsigned short base_port = 10000;

    if(!parse_config_file(&addr, &base_port)) {
        return EXIT_FAILURE;
    }

    spdlog::cfg::load_env_levels(); // load log levels from environment variables
    spdlog::set_pattern("%^[%l]%$ %v");

    // set CPU affinity to big cores, requiring at least 2
    MX::Utils::set_self_affinity_to_big_cores(2);

    // create server object
    server = new MX::Manager::Server(addr, base_port);

    // set up signal handler for ctrl+c
    std::signal(SIGINT, signal_handler);

    // start the server
    server->start();

    // sleep the main thread forever
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return EXIT_SUCCESS;
}
