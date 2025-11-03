// Copyright (c) 2025 MemryX
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#pragma once
#include <vector>
#include <thread>
#include <tuple>
#include <mutex>
#include <condition_variable>
#include <algorithm>

#include <memx/accl/utils/blocky_queue.h>
#include <memx/accl/utils/locked_var.h>

using namespace MX::Utils;

class Task
{
  public:
    virtual bool execute() = 0; // if execute returns true we put it back on the task queue
    virtual ~Task() {}
};

template <typename F, typename... Args>
class CallbackTask: public Task
{
  public:
    template <typename FnT, typename... Ts>
    CallbackTask(FnT &&function, Ts &&... args): func(std::forward<FnT>(function)), args(std::forward<Ts>(args)...) {}
    bool execute() override
    {
        return std::apply(func, args);
    }
  private:
    F func;
    std::tuple<Args...> args;
};

class thread_pool
{
  public:
    thread_pool(const std::string &label, size_t workers, size_t continuous, size_t max_jobs = 0);
    template <typename F, typename... Args>
    void submitTask(F &&function, Args &&... args);
    void wait();
    void stop();
    bool stopped();

  private:
    void workerTarget();
    std::string m_label;
    size_t m_task_count{0};
    size_t m_done_count{0};
    unsigned int m_timeout = 50; // milliseconds
    SharedLockedVar<bool> m_stop{false};
    std::mutex m_mutex;
    std::condition_variable m_done_condition;
    bool m_continuous;
    BlockyQueue<Task*> m_task_queue;
    std::vector<std::thread> m_workers;
};

using namespace std::chrono_literals;

inline thread_pool::thread_pool(const std::string &label,
                                size_t workers, size_t continuous, size_t max_jobs):
    m_label(label),
    m_continuous(continuous),
    m_task_queue(BlockyQueue<Task*>(max_jobs))
{
    for (size_t i = 0; i < workers; ++i) {
        m_workers.push_back(std::thread(&thread_pool::workerTarget, this));
        // TODO try setting thread scheduling priority
    }
}

inline void thread_pool::workerTarget()
{
    Task *ptask = nullptr;
    while (m_stop == false) {
        if(m_task_queue.pop_timeout(ptask, m_timeout) == false){
            continue;
        }

        if(m_continuous) {
            if (ptask->execute()) {
                while(m_task_queue.push_timeout(ptask, m_timeout) == false) {
                    if (m_stop == true) {
                        return;
                    }
                }
                continue;
            }
            delete ptask;
            ptask = nullptr;
            {
                std::lock_guard lock(m_mutex);
                m_done_count++;
                if (m_done_count == m_task_count) {
                    m_done_condition.notify_one();
                }
            }
        }
        else {
            ptask->execute();
            delete ptask;
            ptask = nullptr;
        }
    }
}

inline void thread_pool::stop()
{
    Task *ptask = nullptr;
    if(m_stop == true) {
        return;
    }
    m_stop = true;
    for (auto &worker : m_workers) {
        worker.join();
    }
    if(m_continuous) {
        while(m_done_count < m_task_count) {
            m_task_queue.pop(ptask);
            if (ptask == nullptr) {
                break;
            }
            delete ptask;
            ptask = nullptr;
            m_done_count++;
        }
    }
}

inline bool thread_pool::stopped()
{
    return std::none_of(m_workers.begin(), m_workers.end(), [](const std::thread & th) { return th.joinable(); });
}

inline void thread_pool::wait()
{
    if (m_task_count == 0) {
        return;
    }
    std::unique_lock lock(m_mutex);
    m_done_condition.wait(lock, [this]() { return m_done_count == m_task_count; });
    lock.unlock();
}

template <typename F, typename... Args>
inline void thread_pool::submitTask(F &&function, Args &&... args)
{
    Task* pt = new CallbackTask<F, Args...>(std::forward<F>(function), std::forward<Args>(args)...);
    m_task_queue.push(pt);
    {
        std::lock_guard lock(m_mutex);
        m_task_count++;
    }
}

#endif
