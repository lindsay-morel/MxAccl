//
// detail/impl/reactive_descriptor_service.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_IMPL_REACTIVE_DESCRIPTOR_SERVICE_IPP
#define MXASIO_DETAIL_IMPL_REACTIVE_DESCRIPTOR_SERVICE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if !defined(MXASIO_WINDOWS) \
  && !defined(MXASIO_WINDOWS_RUNTIME) \
  && !defined(__CYGWIN__) \
  && !defined(MXASIO_HAS_IO_URING_AS_DEFAULT)

#include "mxasio/error.hpp"
#include "mxasio/detail/reactive_descriptor_service.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

reactive_descriptor_service::reactive_descriptor_service(
    execution_context& context)
  : execution_context_service_base<reactive_descriptor_service>(context),
    reactor_(mxasio::use_service<reactor>(context))
{
  reactor_.init_task();
}

void reactive_descriptor_service::shutdown()
{
}

void reactive_descriptor_service::construct(
    reactive_descriptor_service::implementation_type& impl)
{
  impl.descriptor_ = -1;
  impl.state_ = 0;
  impl.reactor_data_ = reactor::per_descriptor_data();
}

void reactive_descriptor_service::move_construct(
    reactive_descriptor_service::implementation_type& impl,
    reactive_descriptor_service::implementation_type& other_impl)
  noexcept
{
  impl.descriptor_ = other_impl.descriptor_;
  other_impl.descriptor_ = -1;

  impl.state_ = other_impl.state_;
  other_impl.state_ = 0;

  reactor_.move_descriptor(impl.descriptor_,
      impl.reactor_data_, other_impl.reactor_data_);
}

void reactive_descriptor_service::move_assign(
    reactive_descriptor_service::implementation_type& impl,
    reactive_descriptor_service& other_service,
    reactive_descriptor_service::implementation_type& other_impl)
{
  destroy(impl);

  impl.descriptor_ = other_impl.descriptor_;
  other_impl.descriptor_ = -1;

  impl.state_ = other_impl.state_;
  other_impl.state_ = 0;

  other_service.reactor_.move_descriptor(impl.descriptor_,
      impl.reactor_data_, other_impl.reactor_data_);
}

void reactive_descriptor_service::destroy(
    reactive_descriptor_service::implementation_type& impl)
{
  if (is_open(impl))
  {
    MXASIO_HANDLER_OPERATION((reactor_.context(),
          "descriptor", &impl, impl.descriptor_, "close"));

    reactor_.deregister_descriptor(impl.descriptor_, impl.reactor_data_,
        (impl.state_ & descriptor_ops::possible_dup) == 0);

    mxasio::error_code ignored_ec;
    descriptor_ops::close(impl.descriptor_, impl.state_, ignored_ec);

    reactor_.cleanup_descriptor_data(impl.reactor_data_);
  }
}

mxasio::error_code reactive_descriptor_service::assign(
    reactive_descriptor_service::implementation_type& impl,
    const native_handle_type& native_descriptor, mxasio::error_code& ec)
{
  if (is_open(impl))
  {
    ec = mxasio::error::already_open;
    MXASIO_ERROR_LOCATION(ec);
    return ec;
  }

  if (int err = reactor_.register_descriptor(
        native_descriptor, impl.reactor_data_))
  {
    ec = mxasio::error_code(err,
        mxasio::error::get_system_category());
    MXASIO_ERROR_LOCATION(ec);
    return ec;
  }

  impl.descriptor_ = native_descriptor;
  impl.state_ = descriptor_ops::possible_dup;
  ec = mxasio::error_code();
  return ec;
}

mxasio::error_code reactive_descriptor_service::close(
    reactive_descriptor_service::implementation_type& impl,
    mxasio::error_code& ec)
{
  if (is_open(impl))
  {
    MXASIO_HANDLER_OPERATION((reactor_.context(),
          "descriptor", &impl, impl.descriptor_, "close"));

    reactor_.deregister_descriptor(impl.descriptor_, impl.reactor_data_,
        (impl.state_ & descriptor_ops::possible_dup) == 0);

    descriptor_ops::close(impl.descriptor_, impl.state_, ec);

    reactor_.cleanup_descriptor_data(impl.reactor_data_);
  }
  else
  {
    ec = mxasio::error_code();
  }

  // The descriptor is closed by the OS even if close() returns an error.
  //
  // (Actually, POSIX says the state of the descriptor is unspecified. On
  // Linux the descriptor is apparently closed anyway; e.g. see
  //   http://lkml.org/lkml/2005/9/10/129
  // We'll just have to assume that other OSes follow the same behaviour.)
  construct(impl);

  MXASIO_ERROR_LOCATION(ec);
  return ec;
}

reactive_descriptor_service::native_handle_type
reactive_descriptor_service::release(
    reactive_descriptor_service::implementation_type& impl)
{
  native_handle_type descriptor = impl.descriptor_;

  if (is_open(impl))
  {
    MXASIO_HANDLER_OPERATION((reactor_.context(),
          "descriptor", &impl, impl.descriptor_, "release"));

    reactor_.deregister_descriptor(impl.descriptor_, impl.reactor_data_, false);
    reactor_.cleanup_descriptor_data(impl.reactor_data_);
    construct(impl);
  }

  return descriptor;
}

mxasio::error_code reactive_descriptor_service::cancel(
    reactive_descriptor_service::implementation_type& impl,
    mxasio::error_code& ec)
{
  if (!is_open(impl))
  {
    ec = mxasio::error::bad_descriptor;
    MXASIO_ERROR_LOCATION(ec);
    return ec;
  }

  MXASIO_HANDLER_OPERATION((reactor_.context(),
        "descriptor", &impl, impl.descriptor_, "cancel"));

  reactor_.cancel_ops(impl.descriptor_, impl.reactor_data_);
  ec = mxasio::error_code();
  return ec;
}

void reactive_descriptor_service::do_start_op(implementation_type& impl,
    int op_type, reactor_op* op, bool is_continuation, bool is_non_blocking,
    bool noop, void (*on_immediate)(operation* op, bool, const void*),
    const void* immediate_arg)
{
  if (!noop)
  {
    if ((impl.state_ & descriptor_ops::non_blocking) ||
        descriptor_ops::set_internal_non_blocking(
          impl.descriptor_, impl.state_, true, op->ec_))
    {
      reactor_.start_op(op_type, impl.descriptor_, impl.reactor_data_, op,
          is_continuation, is_non_blocking, on_immediate, immediate_arg);
      return;
    }
  }

  on_immediate(op, is_continuation, immediate_arg);
}

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // !defined(MXASIO_WINDOWS)
       //   && !defined(MXASIO_WINDOWS_RUNTIME)
       //   && !defined(__CYGWIN__)
       //   && !defined(MXASIO_HAS_IO_URING_AS_DEFAULT)

#endif // MXASIO_DETAIL_IMPL_REACTIVE_DESCRIPTOR_SERVICE_IPP
