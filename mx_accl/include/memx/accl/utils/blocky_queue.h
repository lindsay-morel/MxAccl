// Copyright (c) 2025 MemryX
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BLOCKY_QUEUE_H
#define BLOCKY_QUEUE_H

#pragma once
#include <deque>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <limits.h>

#include <memx/accl/utils/macros.h>


namespace MX
{
namespace RPC
{

template<typename T>
class BlockyQueue
{

  public:
    BlockyQueue()
    {
        max_size = UINT_MAX;
        kill = false;
    }
    BlockyQueue(unsigned int capacity)
    {
        max_size = capacity;
        kill = false;
    }
    ~BlockyQueue()
    {
        std::unique_lock<std::mutex> lock(m);
        kill = true;
        q.clear();
        s_not_empty.notify_all();
        s_not_full.notify_all();
        lock.unlock();
    }

    T pop()
    {
        // sleep on s_not_empty until there's data again
        std::unique_lock<std::mutex> lock(m);
        s_not_empty.wait(lock, [this] { return (!q.empty()) || kill; });
        if(UNLIKELY(q.empty())) { return T(); }

        // pop
        T ret = std::move(q.front());
        q.pop_front();

        // wake up anyone waiting on full
        s_not_full.notify_one();

        // clear lock
        lock.unlock();

        return ret;
    }

    void pop(T &ret)
    {
        // sleep on s_not_empty until there's data again
        std::unique_lock<std::mutex> lock(m);
        s_not_empty.wait(lock, [this] { return (!q.empty()) || kill; });
        if(UNLIKELY(q.empty())) { return; } // return if kill was true

        // pop
        ret = std::move(q.front());
        q.pop_front();

        // wake up anyone waiting on full
        s_not_full.notify_one();

        // clear lock
        lock.unlock();
    }

    // pop with a timeout option
    bool pop_timeout(T &ret, unsigned int timeout_ms)
    {
        std::unique_lock<std::mutex> lock(m);
        bool got_data = s_not_empty.wait_for(
                            lock,
                            std::chrono::milliseconds(timeout_ms),
                            [this] { return (!q.empty()) || kill; }
                        );
        if(UNLIKELY(!got_data)) { return false; }
        if(UNLIKELY(q.empty())) { return false; }
        ret = std::move(q.front());
        q.pop_front();
        s_not_full.notify_one();
        lock.unlock();
        return true;
    }

    void push(T &d)
    {
        // sleep on s_not_full until there's space again
        std::unique_lock<std::mutex> lock(m);
        s_not_full.wait(lock, [this] { return (q.size() < max_size) || kill; });
        if(UNLIKELY(q.size() >= max_size)) { return; } // return if kill was true

        // push
        q.push_back(std::move(d));

        // wake up those waiting on empty
        s_not_empty.notify_one();

        // clear lock
        lock.unlock();
    }

    // timed push: waits up to timeout_sec seconds, returns false on timeout
    bool push_timeout(T &d, unsigned int timeout_ms)
    {
        std::unique_lock<std::mutex> lock(m);
        bool has_space = s_not_full.wait_for(
                             lock,
                             std::chrono::milliseconds(timeout_ms),
                             [this] { return (q.size() < max_size) || kill; }
                         );
        if(UNLIKELY(!has_space)) { return false; }           // timeout
        if(UNLIKELY(q.size() >= max_size)) { return false; } // kill flag was set
        q.push_back(std::move(d));
        s_not_empty.notify_one();
        lock.unlock();
        return true;
    }

    unsigned int size() const
    {
        std::lock_guard<std::mutex> lock(m);
        return q.size();
    }

  protected:
    std::deque<T> q;
    mutable std::mutex m;
    mutable std::condition_variable s_not_empty;
    mutable std::condition_variable s_not_full;

    unsigned int max_size;
    bool kill;
};



// BlockyQueue with an additional external wait flag for pop operations
template<typename T>
class BQExtFlag
{

  public:
    explicit BQExtFlag(unsigned int capacity, std::atomic_bool* ext_flag_, bool val_to_wait_for_ = true)
    {
        ext_flag = ext_flag_;
        val_to_wait_for = val_to_wait_for_;
        max_size = capacity;
        kill = false;
    }

    ~BQExtFlag()
    {
        std::unique_lock<std::mutex> lock(m);
        kill = true;
        q.clear();
        s_not_empty.notify_all();
        s_not_full.notify_all();
        lock.unlock();
    }

    T pop()
    {
        // sleep on the CV until there's data again OR the ext_flag is true
        std::unique_lock<std::mutex> lock(m);
        s_not_empty.wait(lock, [this] { return (!(q.empty())) || (ext_flag->load(std::memory_order_consume) == val_to_wait_for) || kill; });
        TSAN_ACQUIRE(ext_flag);

        // we were either killed or the external flag was set
        if(UNLIKELY(q.empty())) { return T(); } // return default-constructed T

        // pop
        T ret = std::move(q.front());
        q.pop_front();

        // wake up anyone waiting on full
        s_not_full.notify_one();

        // clear lock
        lock.unlock();

        return ret; // successfully popped data
    }

    bool pop(T &ret)
    {
        // sleep on the CV until there's data again OR the ext_flag is true
        std::unique_lock<std::mutex> lock(m);
        s_not_empty.wait(lock, [this] { return (!(q.empty())) || (ext_flag->load(std::memory_order_consume) == val_to_wait_for) || kill; });
        TSAN_ACQUIRE(ext_flag);

        // we were either killed or the external flag was set
        if(UNLIKELY(q.empty())) {
            lock.unlock();
            return false;
        }

        // pop
        ret = std::move(q.front());
        q.pop_front();

        // wake up anyone waiting on full
        s_not_full.notify_one();

        // clear lock
        lock.unlock();

        return true; // successfully popped data
    }

    // keep popping until q is empty
    // ignore flag values -- the spice must flow!
    bool drain_pop(T &ret)
    {
        std::unique_lock<std::mutex> lock(m);

        if(q.empty()) {
            lock.unlock();
            return false; // queue was empty
        }
        ret = std::move(q.front());
        q.pop_front();
        s_not_full.notify_one();
        lock.unlock();
        return true; // successfully drained the queue
    }

    // pop with a timeout option (wait until: there's data, or timeout, or ext_flag is set true)
    bool pop_timeout(T &ret, unsigned int timeout_ms)
    {
        std::unique_lock<std::mutex> lock(m);
        bool got_data = s_not_empty.wait_for(
                            lock,
                            std::chrono::milliseconds(timeout_ms),
                            [this] { return (!(q.empty())) || (ext_flag->load(std::memory_order_consume) == val_to_wait_for) || kill; }
                        );
        TSAN_ACQUIRE(ext_flag);
        if(q.empty() || !got_data) {
            lock.unlock();
            return false; // timeout or external flag is set
        }
        ret = std::move(q.front());
        q.pop_front();

        // wake up anyone waiting on full
        s_not_full.notify_one();

        lock.unlock();

        return true;
    }

    void push(T &d)
    {
        // sleep on s_not_full until there's space again
        std::unique_lock<std::mutex> lock(m);
        s_not_full.wait(lock, [this] { return (q.size() < max_size) || kill; });
        if(UNLIKELY(kill)) { return; } // return if kill was true

        // push
        q.push_back(std::move(d));

        // wake up those waiting on empty
        s_not_empty.notify_one();

        // clear lock
        lock.unlock();
    }

    // timed push: waits up to timeout_sec seconds, returns false on timeout
    bool push_timeout(T &d, unsigned int timeout_ms)
    {
        std::unique_lock<std::mutex> lock(m);
        bool has_space = s_not_full.wait_for(
                             lock,
                             std::chrono::milliseconds(timeout_ms),
                             [this] { return (q.size() < max_size) || kill; }
                         );
        if(UNLIKELY(!has_space)) { return false; } // timeout
        if(UNLIKELY(kill)) { return false; } // kill flag was set
        q.push_back(std::move(d));
        s_not_empty.notify_one();
        lock.unlock();
        return true;
    }

    unsigned int size()
    {
        std::lock_guard<std::mutex> lock(m);
        return q.size();
    }

    void notify()
    {
        std::unique_lock<std::mutex> lock(m);
        s_not_empty.notify_all(); // wake up anyone waiting on the pop CV
        lock.unlock();
    }

  private:
    std::deque<T> q;
    mutable std::mutex m;
    mutable std::condition_variable s_not_empty;
    mutable std::condition_variable s_not_full;
    unsigned int max_size;
    bool kill;


    bool val_to_wait_for; // value to wait for in the external flag
    std::atomic_bool* ext_flag; // external wait flag, if set to true, pop will block until it is set to false
};

}
}

#endif
