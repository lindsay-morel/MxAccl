//
// detail/winrt_timer_scheduler.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_WINRT_TIMER_SCHEDULER_HPP
#define MXASIO_DETAIL_WINRT_TIMER_SCHEDULER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_WINDOWS_RUNTIME)

#include <cstddef>
#include "mxasio/detail/event.hpp"
#include "mxasio/detail/limits.hpp"
#include "mxasio/detail/mutex.hpp"
#include "mxasio/detail/op_queue.hpp"
#include "mxasio/detail/thread.hpp"
#include "mxasio/detail/timer_queue_base.hpp"
#include "mxasio/detail/timer_queue_set.hpp"
#include "mxasio/detail/wait_op.hpp"
#include "mxasio/execution_context.hpp"

#if defined(MXASIO_HAS_IOCP)
#include "mxasio/detail/win_iocp_io_context.hpp"
#else // defined(MXASIO_HAS_IOCP)
#include "mxasio/detail/scheduler.hpp"
#endif // defined(MXASIO_HAS_IOCP)

#if defined(MXASIO_HAS_IOCP)
#include "mxasio/detail/thread.hpp"
#endif // defined(MXASIO_HAS_IOCP)

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

class winrt_timer_scheduler
  : public execution_context_service_base<winrt_timer_scheduler>
{
public:
  // Constructor.
  MXASIO_DECL winrt_timer_scheduler(execution_context& context);

  // Destructor.
  MXASIO_DECL ~winrt_timer_scheduler();

  // Destroy all user-defined handler objects owned by the service.
  MXASIO_DECL void shutdown();

  // Recreate internal descriptors following a fork.
  MXASIO_DECL void notify_fork(execution_context::fork_event fork_ev);

  // Initialise the task. No effect as this class uses its own thread.
  MXASIO_DECL void init_task();

  // Add a new timer queue to the reactor.
  template <typename Time_Traits>
  void add_timer_queue(timer_queue<Time_Traits>& queue);

  // Remove a timer queue from the reactor.
  template <typename Time_Traits>
  void remove_timer_queue(timer_queue<Time_Traits>& queue);

  // Schedule a new operation in the given timer queue to expire at the
  // specified absolute time.
  template <typename Time_Traits>
  void schedule_timer(timer_queue<Time_Traits>& queue,
      const typename Time_Traits::time_type& time,
      typename timer_queue<Time_Traits>::per_timer_data& timer, wait_op* op);

  // Cancel the timer operations associated with the given token. Returns the
  // number of operations that have been posted or dispatched.
  template <typename Time_Traits>
  std::size_t cancel_timer(timer_queue<Time_Traits>& queue,
      typename timer_queue<Time_Traits>::per_timer_data& timer,
      std::size_t max_cancelled = (std::numeric_limits<std::size_t>::max)());

  // Move the timer operations associated with the given timer.
  template <typename Time_Traits>
  void move_timer(timer_queue<Time_Traits>& queue,
      typename timer_queue<Time_Traits>::per_timer_data& to,
      typename timer_queue<Time_Traits>::per_timer_data& from);

private:
  // Run the select loop in the thread.
  MXASIO_DECL void run_thread();

  // Entry point for the select loop thread.
  MXASIO_DECL static void call_run_thread(winrt_timer_scheduler* reactor);

  // Helper function to add a new timer queue.
  MXASIO_DECL void do_add_timer_queue(timer_queue_base& queue);

  // Helper function to remove a timer queue.
  MXASIO_DECL void do_remove_timer_queue(timer_queue_base& queue);

  // The scheduler implementation used to post completions.
#if defined(MXASIO_HAS_IOCP)
  typedef class win_iocp_io_context scheduler_impl;
#else
  typedef class scheduler scheduler_impl;
#endif
  scheduler_impl& scheduler_;

  // Mutex used to protect internal variables.
  mxasio::detail::mutex mutex_;

  // Event used to wake up background thread.
  mxasio::detail::event event_;

  // The timer queues.
  timer_queue_set timer_queues_;

  // The background thread that is waiting for timers to expire.
  mxasio::detail::thread* thread_;

  // Does the background thread need to stop.
  bool stop_thread_;

  // Whether the service has been shut down.
  bool shutdown_;
};

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#include "mxasio/detail/impl/winrt_timer_scheduler.hpp"
#if defined(MXASIO_HEADER_ONLY)
#include "mxasio/detail/impl/winrt_timer_scheduler.ipp"
#endif // defined(MXASIO_HEADER_ONLY)

#endif // defined(MXASIO_WINDOWS_RUNTIME)

#endif // MXASIO_DETAIL_WINRT_TIMER_SCHEDULER_HPP
