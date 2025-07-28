//
// impl/io_context.ipp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_IMPL_IO_CONTEXT_IPP
#define MXASIO_IMPL_IO_CONTEXT_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include "mxasio/io_context.hpp"
#include "mxasio/detail/concurrency_hint.hpp"
#include "mxasio/detail/limits.hpp"
#include "mxasio/detail/scoped_ptr.hpp"
#include "mxasio/detail/service_registry.hpp"
#include "mxasio/detail/throw_error.hpp"

#if defined(MXASIO_HAS_IOCP)
#include "mxasio/detail/win_iocp_io_context.hpp"
#else
#include "mxasio/detail/scheduler.hpp"
#endif

#include "mxasio/detail/push_options.hpp"

namespace mxasio {

io_context::io_context()
  : impl_(add_impl(new impl_type(*this,
          MXASIO_CONCURRENCY_HINT_DEFAULT, false)))
{
}

io_context::io_context(int concurrency_hint)
  : impl_(add_impl(new impl_type(*this, concurrency_hint == 1
          ? MXASIO_CONCURRENCY_HINT_1 : concurrency_hint, false)))
{
}

io_context::impl_type& io_context::add_impl(io_context::impl_type* impl)
{
  mxasio::detail::scoped_ptr<impl_type> scoped_impl(impl);
  mxasio::add_service<impl_type>(*this, scoped_impl.get());
  return *scoped_impl.release();
}

io_context::~io_context()
{
  shutdown();
}

io_context::count_type io_context::run()
{
  mxasio::error_code ec;
  count_type s = impl_.run(ec);
  mxasio::detail::throw_error(ec);
  return s;
}

#if !defined(MXASIO_NO_DEPRECATED)
io_context::count_type io_context::run(mxasio::error_code& ec)
{
  return impl_.run(ec);
}
#endif // !defined(MXASIO_NO_DEPRECATED)

io_context::count_type io_context::run_one()
{
  mxasio::error_code ec;
  count_type s = impl_.run_one(ec);
  mxasio::detail::throw_error(ec);
  return s;
}

#if !defined(MXASIO_NO_DEPRECATED)
io_context::count_type io_context::run_one(mxasio::error_code& ec)
{
  return impl_.run_one(ec);
}
#endif // !defined(MXASIO_NO_DEPRECATED)

io_context::count_type io_context::poll()
{
  mxasio::error_code ec;
  count_type s = impl_.poll(ec);
  mxasio::detail::throw_error(ec);
  return s;
}

#if !defined(MXASIO_NO_DEPRECATED)
io_context::count_type io_context::poll(mxasio::error_code& ec)
{
  return impl_.poll(ec);
}
#endif // !defined(MXASIO_NO_DEPRECATED)

io_context::count_type io_context::poll_one()
{
  mxasio::error_code ec;
  count_type s = impl_.poll_one(ec);
  mxasio::detail::throw_error(ec);
  return s;
}

#if !defined(MXASIO_NO_DEPRECATED)
io_context::count_type io_context::poll_one(mxasio::error_code& ec)
{
  return impl_.poll_one(ec);
}
#endif // !defined(MXASIO_NO_DEPRECATED)

void io_context::stop()
{
  impl_.stop();
}

bool io_context::stopped() const
{
  return impl_.stopped();
}

void io_context::restart()
{
  impl_.restart();
}

io_context::service::service(mxasio::io_context& owner)
  : execution_context::service(owner)
{
}

io_context::service::~service()
{
}

void io_context::service::shutdown()
{
#if !defined(MXASIO_NO_DEPRECATED)
  shutdown_service();
#endif // !defined(MXASIO_NO_DEPRECATED)
}

#if !defined(MXASIO_NO_DEPRECATED)
void io_context::service::shutdown_service()
{
}
#endif // !defined(MXASIO_NO_DEPRECATED)

void io_context::service::notify_fork(io_context::fork_event ev)
{
#if !defined(MXASIO_NO_DEPRECATED)
  fork_service(ev);
#else // !defined(MXASIO_NO_DEPRECATED)
  (void)ev;
#endif // !defined(MXASIO_NO_DEPRECATED)
}

#if !defined(MXASIO_NO_DEPRECATED)
void io_context::service::fork_service(io_context::fork_event)
{
}
#endif // !defined(MXASIO_NO_DEPRECATED)

} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // MXASIO_IMPL_IO_CONTEXT_IPP
