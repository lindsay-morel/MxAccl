// Copyright (c) 2025 MemryX
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef LOCK_TABLE_H
#define LOCK_TABLE_H

#pragma once
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <cstdint>

namespace MX
{
namespace Manager
{

class LockTable
{
  private:
    int num_devices;
    bool*        lock_states;
    uint32_t*    owner_ids;
    std::shared_mutex*  dev_mutexes;
    std::condition_variable_any* dev_cvs;

  public:
    LockTable();
    ~LockTable();

    void init(int n);

    void lock(int32_t dev_id, uint32_t client_id);
    bool trylock(int32_t dev_id, uint32_t client_id);
    bool unlock(int32_t dev_id, uint32_t client_id);
    bool check_lock(int32_t dev_id, uint32_t* owner_id);
    bool clear_client(uint32_t client_id);

    void print();

};

}
}

#endif // LOCK_TABLE_H
