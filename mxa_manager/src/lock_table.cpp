// Copyright (c) 2025 MemryX
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <iostream>
#include <cstdlib>

#include "lock_table.h"

namespace MX
{
namespace Manager
{

// LockTable
//-------------------------------------------------------
LockTable::LockTable()
{
    num_devices = -1;
    lock_states = nullptr;
    owner_ids   = nullptr;
    dev_mutexes = nullptr;
    dev_cvs     = nullptr;
}

LockTable::~LockTable()
{
    delete [] lock_states;
    delete [] owner_ids;
    delete [] dev_mutexes;
    delete [] dev_cvs;
    lock_states = nullptr;
    owner_ids   = nullptr;
    dev_mutexes = nullptr;
    dev_cvs     = nullptr;
}

// Allocate arrays for device lock states, owner ids, mutexes, and condition variables.
void LockTable::init(int n)
{
    num_devices = n;
    if (n <= 0) {
        lock_states = nullptr;
        owner_ids   = nullptr;
        dev_mutexes = nullptr;
        dev_cvs     = nullptr;
    }
    else {
        lock_states = new bool[n];
        owner_ids   = new uint32_t[n];
        for(int i = 0; i < n; i++) {
            lock_states[i] = false; // unlocked
            owner_ids[i] = 0xDEADBEEF; // invalid ID
        }
        dev_mutexes = new std::shared_mutex[n];    // One mutex per device
        dev_cvs     = new std::condition_variable_any[n];  // One condition variable per device
    }
}

// Attempt to acquire the lock for device 'dev_id'
bool LockTable::trylock(int32_t dev_id, uint32_t client_id)
{
    if (dev_id < 0 || dev_id >= num_devices || num_devices <= 0) {
        return false;
    }
    std::unique_lock<std::shared_mutex> lock(dev_mutexes[dev_id]);
    if (lock_states[dev_id]) {
        // Already locked; if the client already owns it, return true.
        return (owner_ids[dev_id] == client_id);
    }
    else {
        owner_ids[dev_id] = client_id;
        lock_states[dev_id] = true;
        return true;
    }
}

// Check if the device is locked and return the owner ID if it is.
bool LockTable::check_lock(int32_t dev_id, uint32_t* owner_id)
{
    if (dev_id < 0 || dev_id >= num_devices || num_devices <= 0) {
        return false;
    }
    std::shared_lock<std::shared_mutex> lock(dev_mutexes[dev_id]);
    if (lock_states[dev_id]) {
        *owner_id = owner_ids[dev_id];
        return true;
    }
    else {
        return false;
    }
}


// Unlock the device and notify only threads waiting for this device.
bool LockTable::unlock(int32_t dev_id, uint32_t client_id)
{
    if (dev_id < 0 || dev_id >= num_devices || num_devices <= 0) {
        return false;
    }
    std::unique_lock<std::shared_mutex> lock(dev_mutexes[dev_id]);
    if (lock_states[dev_id]) {
        if (owner_ids[dev_id] == client_id) {
            lock_states[dev_id] = false;
            owner_ids[dev_id] = 0xDEADBEEF;
            // Notify only threads waiting on this device's condition variable.
            dev_cvs[dev_id].notify_one();
            return true;
        }
        else {
            // The client does not own the lock.
            return false;
        }
    }
    else {
        // The device is already unlocked.
        return true;
    }
}

// Blocking lock: waits until the device becomes available, then acquires it.
void LockTable::lock(int32_t dev_id, uint32_t client_id)
{
    if (dev_id < 0 || dev_id >= num_devices || num_devices <= 0) {
        return;
    }
    std::unique_lock<std::shared_mutex> lock(dev_mutexes[dev_id]);
    // Wait until the lock for dev_id is not held.
    dev_cvs[dev_id].wait(lock, [this, dev_id] { return !lock_states[dev_id]; });
    // Acquire the lock.
    lock_states[dev_id] = true;
    owner_ids[dev_id] = client_id;
    printf("LockTable::lock(%d,%08X) claimed dev\n", dev_id, client_id);
}


void LockTable::print()
{
    printf("Device    State    Owner ID\n");
    printf("-------   ------   --------\n");
    if(num_devices <= 0) {
        printf("No devices found\n");
        return;
    }
    for(int i = 0; i < num_devices; i++) {
        std::shared_lock<std::shared_mutex> lock(dev_mutexes[i]);
        printf("   %d      %s   %08X\n", i, lock_states[i] ? "Locked" : "Free  ", owner_ids[i]);
    }
}


bool LockTable::clear_client(uint32_t client_id)
{
    if(num_devices <= 0) {
        return false;
    }
    bool found = false;
    for(int i = 0; i < num_devices; i++) {
        std::unique_lock<std::shared_mutex> lock(dev_mutexes[i]);
        if(lock_states[i]) {
            if(owner_ids[i] == client_id) {
                printf("Client ID %08X had a dangling lock hold on device %d. Cleared it.\n", client_id, i);
                lock_states[i] = false;
                owner_ids[i] = 0xDEADBEEF;
                dev_cvs[i].notify_one();
                found = true;
            }
        }
    }
    return found;
}


} // namespace Manager
} // namespace MX
