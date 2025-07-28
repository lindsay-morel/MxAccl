// Copyright (c) 2025 MemryX
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef MX_UTILS_GENERAL_H
#define MX_UTILS_GENERAL_H

#pragma once
#include <condition_variable>
#include <mutex>
#include <queue>
#include <optional>

#ifdef __linux__
    #include <unistd.h>
    #include <sys/syscall.h>
#endif

#include <iostream>

// to get the MEMX_API_EXPORT macro
#include <memx/memx.h>

using namespace std;

namespace MX
{
namespace Utils
{
template <typename T>
class fifo_queue
{
  private:
    std::queue<T> m_queue;

  public:
    mutable std::mutex m_mutex;

    size_t size() const
    {
        lock_guard<mutex> lock(m_mutex);
        return m_queue.size();
    }
    bool empty() const
    {
        lock_guard<mutex> lock(m_mutex);
        return m_queue.empty();
    }
    void push(T item)
    {
        lock_guard<mutex> lock(m_mutex);
        m_queue.push(item);
    }
    T pop()
    {
        lock_guard<mutex> lock(m_mutex);
        T item = m_queue.front();
        m_queue.pop();
        return item;
    }
    fifo_queue &operator=(const fifo_queue &rhs) // copy assignment
    {
        if (this != &rhs) {
            lock_guard<mutex> lock(m_mutex);
            m_queue = rhs.m_queue;
        }
        return *this;
    }
};

template <typename T1, typename T2>
class fifo_deque
{
  private:
    // std::deque<T> m_queue;

  public:
    std::mutex m_mutex;
    std::deque<std::pair<T1, T2>> m_queue;

    size_t size()
    {
        lock_guard<mutex> lock(m_mutex);
        return m_queue.size();
    }
    void push(std::pair<T1, T2> item)
    {
        lock_guard<mutex> lock(m_mutex);
        m_queue.push_back(item);
    }
    std::pair<T1, T2> pop()
    {
        lock_guard<mutex> lock(m_mutex);
        std::pair<T1, T2> item = m_queue.front();
        m_queue.pop_front();
        return item;
    }
    std::pair<T1, T2> get()
    {
        lock_guard<mutex> lock(m_mutex);
        std::pair<T1, T2> item = m_queue[0];
        return item;
    }
    std::optional<std::pair<T1, T2>> ifPophold(std::pair<T1, T2> item)
    {
        m_mutex.lock();
        if(m_queue.size() == 0) return {};
        std::pair<T1, T2> front = m_queue[0];
        if(item.first == front.first) {
            m_queue.pop_front();
            return front;
        }
        return {};
    }
};

typedef struct retval {
    bool error_flag;
    std::string error_msg;
    retval(bool flag)
    {
        error_flag = flag;
    }
    retval() {};
} mx_retval;

MEMX_API_EXPORT void mx_checkandthrow(mx_retval ret);
MEMX_API_EXPORT void mx_checkandprint(mx_retval ret);
} // namespace Utils
} // namespace MX

#endif
