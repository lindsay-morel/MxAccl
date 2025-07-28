//
// detail/dev_poll_reactor.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_DEV_POLL_REACTOR_HPP
#define MXASIO_DETAIL_DEV_POLL_REACTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_HAS_DEV_POLL)

#include <cstddef>
#include <vector>
#include <sys/devpoll.h>
#include "mxasio/detail/hash_map.hpp"
#include "mxasio/detail/limits.hpp"
#include "mxasio/detail/mutex.hpp"
#include "mxasio/detail/op_queue.hpp"
#include "mxasio/detail/reactor_op.hpp"
#include "mxasio/detail/reactor_op_queue.hpp"
#include "mxasio/detail/scheduler_task.hpp"
#include "mxasio/detail/select_interrupter.hpp"
#include "mxasio/detail/socket_types.hpp"
#include "mxasio/detail/timer_queue_base.hpp"
#include "mxasio/detail/timer_queue_set.hpp"
#include "mxasio/detail/wait_op.hpp"
#include "mxasio/execution_context.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

class dev_poll_reactor
  : public execution_context_service_base<dev_poll_reactor>,
    public scheduler_task
{
public:
  enum op_types { read_op = 0, write_op = 1,
    connect_op = 1, except_op = 2, max_ops = 3 };

  // Per-descriptor data.
  struct per_descriptor_data
  {
  };

  // Constructor.
  MXASIO_DECL dev_poll_reactor(mxasio::execution_context& ctx);

  // Destructor.
  MXASIO_DECL ~dev_poll_reactor();

  // Destroy all user-defined handler objects owned by the service.
  MXASIO_DECL void shutdown();

  // Recreate internal descriptors following a fork.
  MXASIO_DECL void notify_fork(
      mxasio::execution_context::fork_event fork_ev);

  // Initialise the task.
  MXASIO_DECL void init_task();

  // Register a socket with the reactor. Returns 0 on success, system error
  // code on failure.
  MXASIO_DECL int register_descriptor(socket_type, per_descriptor_data&);

  // Register a descriptor with an associated single operation. Returns 0 on
  // success, system error code on failure.
  MXASIO_DECL int register_internal_descriptor(
      int op_type, socket_type descriptor,
      per_descriptor_data& descriptor_data, reactor_op* op);

  // Move descriptor registration from one descriptor_data object to another.
  MXASIO_DECL void move_descriptor(socket_type descriptor,
      per_descriptor_data& target_descriptor_data,
      per_descriptor_data& source_descriptor_data);

  // Post a reactor operation for immediate completion.
  void post_immediate_completion(operation* op, bool is_continuation) const;

  // Post a reactor operation for immediate completion.
  MXASIO_DECL static void call_post_immediate_completion(
      operation* op, bool is_continuation, const void* self);

  // Start a new operation. The reactor operation will be performed when the
  // given descriptor is flagged as ready, or an error has occurred.
  MXASIO_DECL void start_op(int op_type, socket_type descriptor,
      per_descriptor_data&, reactor_op* op,
      bool is_continuation, bool allow_speculative,
      void (*on_immediate)(operation*, bool, const void*),
      const void* immediate_arg);

  // Start a new operation. The reactor operation will be performed when the
  // given descriptor is flagged as ready, or an error has occurred.
  void start_op(int op_type, socket_type descriptor,
      per_descriptor_data& descriptor_data, reactor_op* op,
      bool is_continuation, bool allow_speculative)
  {
    start_op(op_type, descriptor, descriptor_data,
        op, is_continuation, allow_speculative,
        &dev_poll_reactor::call_post_immediate_completion, this);
  }

  // Cancel all operations associated with the given descriptor. The
  // handlers associated with the descriptor will be invoked with the
  // operation_aborted error.
  MXASIO_DECL void cancel_ops(socket_type descriptor, per_descriptor_data&);

  // Cancel all operations associated with the given descriptor and key. The
  // handlers associated with the descriptor will be invoked with the
  // operation_aborted error.
  MXASIO_DECL void cancel_ops_by_key(socket_type descriptor,
      per_descriptor_data& descriptor_data,
      int op_type, void* cancellation_key);

  // Cancel any operations that are running against the descriptor and remove
  // its registration from the reactor. The reactor resources associated with
  // the descriptor must be released by calling cleanup_descriptor_data.
  MXASIO_DECL void deregister_descriptor(socket_type descriptor,
      per_descriptor_data&, bool closing);

  // Remove the descriptor's registration from the reactor. The reactor
  // resources associated with the descriptor must be released by calling
  // cleanup_descriptor_data.
  MXASIO_DECL void deregister_internal_descriptor(
      socket_type descriptor, per_descriptor_data&);

  // Perform any post-deregistration cleanup tasks associated with the
  // descriptor data.
  MXASIO_DECL void cleanup_descriptor_data(per_descriptor_data&);

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

  // Cancel the timer operations associated with the given key.
  template <typename Time_Traits>
  void cancel_timer_by_key(timer_queue<Time_Traits>& queue,
      typename timer_queue<Time_Traits>::per_timer_data* timer,
      void* cancellation_key);

  // Move the timer operations associated with the given timer.
  template <typename Time_Traits>
  void move_timer(timer_queue<Time_Traits>& queue,
      typename timer_queue<Time_Traits>::per_timer_data& target,
      typename timer_queue<Time_Traits>::per_timer_data& source);

  // Run /dev/poll once until interrupted or events are ready to be dispatched.
  MXASIO_DECL void run(long usec, op_queue<operation>& ops);

  // Interrupt the select loop.
  MXASIO_DECL void interrupt();

private:
  // Create the /dev/poll file descriptor. Throws an exception if the descriptor
  // cannot be created.
  MXASIO_DECL static int do_dev_poll_create();

  // Helper function to add a new timer queue.
  MXASIO_DECL void do_add_timer_queue(timer_queue_base& queue);

  // Helper function to remove a timer queue.
  MXASIO_DECL void do_remove_timer_queue(timer_queue_base& queue);

  // Get the timeout value for the /dev/poll DP_POLL operation. The timeout
  // value is returned as a number of milliseconds. A return value of -1
  // indicates that the poll should block indefinitely.
  MXASIO_DECL int get_timeout(int msec);

  // Cancel all operations associated with the given descriptor. The do_cancel
  // function of the handler objects will be invoked. This function does not
  // acquire the dev_poll_reactor's mutex.
  MXASIO_DECL void cancel_ops_unlocked(socket_type descriptor,
      const mxasio::error_code& ec);

  // Add a pending event entry for the given descriptor.
  MXASIO_DECL ::pollfd& add_pending_event_change(int descriptor);

  // The scheduler implementation used to post completions.
  scheduler& scheduler_;

  // Mutex to protect access to internal data.
  mxasio::detail::mutex mutex_;

  // The /dev/poll file descriptor.
  int dev_poll_fd_;

  // Vector of /dev/poll events waiting to be written to the descriptor.
  std::vector< ::pollfd> pending_event_changes_;

  // Hash map to associate a descriptor with a pending event change index.
  hash_map<int, std::size_t> pending_event_change_index_;

  // The interrupter is used to break a blocking DP_POLL operation.
  select_interrupter interrupter_;

  // The queues of read, write and except operations.
  reactor_op_queue<socket_type> op_queue_[max_ops];

  // The timer queues.
  timer_queue_set timer_queues_;

  // Whether the service has been shut down.
  bool shutdown_;
};

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#include "mxasio/detail/impl/dev_poll_reactor.hpp"
#if defined(MXASIO_HEADER_ONLY)
#include "mxasio/detail/impl/dev_poll_reactor.ipp"
#endif // defined(MXASIO_HEADER_ONLY)

#endif // defined(MXASIO_HAS_DEV_POLL)

#endif // MXASIO_DETAIL_DEV_POLL_REACTOR_HPP
