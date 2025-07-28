//
// detail/resolver_service_base.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_RESOLVER_SERVICE_BASE_HPP
#define MXASIO_DETAIL_RESOLVER_SERVICE_BASE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include "mxasio/error.hpp"
#include "mxasio/execution_context.hpp"
#include "mxasio/detail/mutex.hpp"
#include "mxasio/detail/noncopyable.hpp"
#include "mxasio/detail/resolve_op.hpp"
#include "mxasio/detail/socket_ops.hpp"
#include "mxasio/detail/socket_types.hpp"
#include "mxasio/detail/scoped_ptr.hpp"
#include "mxasio/detail/thread.hpp"

#if defined(MXASIO_HAS_IOCP)
#include "mxasio/detail/win_iocp_io_context.hpp"
#else // defined(MXASIO_HAS_IOCP)
#include "mxasio/detail/scheduler.hpp"
#endif // defined(MXASIO_HAS_IOCP)

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

class resolver_service_base
{
public:
  // The implementation type of the resolver. A cancellation token is used to
  // indicate to the background thread that the operation has been cancelled.
  typedef socket_ops::shared_cancel_token_type implementation_type;

  // Constructor.
  MXASIO_DECL resolver_service_base(execution_context& context);

  // Destructor.
  MXASIO_DECL ~resolver_service_base();

  // Destroy all user-defined handler objects owned by the service.
  MXASIO_DECL void base_shutdown();

  // Perform any fork-related housekeeping.
  MXASIO_DECL void base_notify_fork(
      execution_context::fork_event fork_ev);

  // Construct a new resolver implementation.
  MXASIO_DECL void construct(implementation_type& impl);

  // Destroy a resolver implementation.
  MXASIO_DECL void destroy(implementation_type&);

  // Move-construct a new resolver implementation.
  MXASIO_DECL void move_construct(implementation_type& impl,
      implementation_type& other_impl);

  // Move-assign from another resolver implementation.
  MXASIO_DECL void move_assign(implementation_type& impl,
      resolver_service_base& other_service,
      implementation_type& other_impl);

  // Move-construct a new timer implementation.
  void converting_move_construct(implementation_type& impl,
      resolver_service_base&, implementation_type& other_impl)
  {
    move_construct(impl, other_impl);
  }

  // Move-assign from another timer implementation.
  void converting_move_assign(implementation_type& impl,
      resolver_service_base& other_service,
      implementation_type& other_impl)
  {
    move_assign(impl, other_service, other_impl);
  }

  // Cancel pending asynchronous operations.
  MXASIO_DECL void cancel(implementation_type& impl);

protected:
  // Helper function to start an asynchronous resolve operation.
  MXASIO_DECL void start_resolve_op(resolve_op* op);

#if !defined(MXASIO_WINDOWS_RUNTIME)
  // Helper class to perform exception-safe cleanup of addrinfo objects.
  class auto_addrinfo
    : private mxasio::detail::noncopyable
  {
  public:
    explicit auto_addrinfo(mxasio::detail::addrinfo_type* ai)
      : ai_(ai)
    {
    }

    ~auto_addrinfo()
    {
      if (ai_)
        socket_ops::freeaddrinfo(ai_);
    }

    operator mxasio::detail::addrinfo_type*()
    {
      return ai_;
    }

  private:
    mxasio::detail::addrinfo_type* ai_;
  };
#endif // !defined(MXASIO_WINDOWS_RUNTIME)

  // Helper class to run the work scheduler in a thread.
  class work_scheduler_runner;

  // Start the work scheduler if it's not already running.
  MXASIO_DECL void start_work_thread();

  // The scheduler implementation used to post completions.
#if defined(MXASIO_HAS_IOCP)
  typedef class win_iocp_io_context scheduler_impl;
#else
  typedef class scheduler scheduler_impl;
#endif
  scheduler_impl& scheduler_;

private:
  // Mutex to protect access to internal data.
  mxasio::detail::mutex mutex_;

  // Private scheduler used for performing asynchronous host resolution.
  mxasio::detail::scoped_ptr<scheduler_impl> work_scheduler_;

  // Thread used for running the work io_context's run loop.
  mxasio::detail::scoped_ptr<mxasio::detail::thread> work_thread_;
};

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#if defined(MXASIO_HEADER_ONLY)
#include "mxasio/detail/impl/resolver_service_base.ipp"
#endif // defined(MXASIO_HEADER_ONLY)

#endif // MXASIO_DETAIL_RESOLVER_SERVICE_BASE_HPP
