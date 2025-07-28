//
// detail/descriptor_read_op.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_DESCRIPTOR_READ_OP_HPP
#define MXASIO_DETAIL_DESCRIPTOR_READ_OP_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if !defined(MXASIO_WINDOWS) && !defined(__CYGWIN__)

#include "mxasio/detail/bind_handler.hpp"
#include "mxasio/detail/buffer_sequence_adapter.hpp"
#include "mxasio/detail/descriptor_ops.hpp"
#include "mxasio/detail/fenced_block.hpp"
#include "mxasio/detail/handler_work.hpp"
#include "mxasio/detail/memory.hpp"
#include "mxasio/detail/reactor_op.hpp"
#include "mxasio/dispatch.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

template <typename MutableBufferSequence>
class descriptor_read_op_base : public reactor_op
{
public:
  descriptor_read_op_base(const mxasio::error_code& success_ec,
      int descriptor, const MutableBufferSequence& buffers,
      func_type complete_func)
    : reactor_op(success_ec,
        &descriptor_read_op_base::do_perform, complete_func),
      descriptor_(descriptor),
      buffers_(buffers)
  {
  }

  static status do_perform(reactor_op* base)
  {
    MXASIO_ASSUME(base != 0);
    descriptor_read_op_base* o(static_cast<descriptor_read_op_base*>(base));

    typedef buffer_sequence_adapter<mxasio::mutable_buffer,
        MutableBufferSequence> bufs_type;

    status result;
    if (bufs_type::is_single_buffer)
    {
      result = descriptor_ops::non_blocking_read1(o->descriptor_,
          bufs_type::first(o->buffers_).data(),
          bufs_type::first(o->buffers_).size(),
          o->ec_, o->bytes_transferred_) ? done : not_done;
    }
    else
    {
      bufs_type bufs(o->buffers_);
      result = descriptor_ops::non_blocking_read(o->descriptor_,
          bufs.buffers(), bufs.count(), o->ec_, o->bytes_transferred_)
        ? done : not_done;
    }

    MXASIO_HANDLER_REACTOR_OPERATION((*o, "non_blocking_read",
          o->ec_, o->bytes_transferred_));

    return result;
  }

private:
  int descriptor_;
  MutableBufferSequence buffers_;
};

template <typename MutableBufferSequence, typename Handler, typename IoExecutor>
class descriptor_read_op
  : public descriptor_read_op_base<MutableBufferSequence>
{
public:
  typedef Handler handler_type;
  typedef IoExecutor io_executor_type;

  MXASIO_DEFINE_HANDLER_PTR(descriptor_read_op);

  descriptor_read_op(const mxasio::error_code& success_ec,
      int descriptor, const MutableBufferSequence& buffers,
      Handler& handler, const IoExecutor& io_ex)
    : descriptor_read_op_base<MutableBufferSequence>(success_ec,
        descriptor, buffers, &descriptor_read_op::do_complete),
      handler_(static_cast<Handler&&>(handler)),
      work_(handler_, io_ex)
  {
  }

  static void do_complete(void* owner, operation* base,
      const mxasio::error_code& /*ec*/,
      std::size_t /*bytes_transferred*/)
  {
    // Take ownership of the handler object.
    MXASIO_ASSUME(base != 0);
    descriptor_read_op* o(static_cast<descriptor_read_op*>(base));
    ptr p = { mxasio::detail::addressof(o->handler_), o, o };

    MXASIO_HANDLER_COMPLETION((*o));

    // Take ownership of the operation's outstanding work.
    handler_work<Handler, IoExecutor> w(
        static_cast<handler_work<Handler, IoExecutor>&&>(
          o->work_));

    MXASIO_ERROR_LOCATION(o->ec_);

    // Make a copy of the handler so that the memory can be deallocated before
    // the upcall is made. Even if we're not about to make an upcall, a
    // sub-object of the handler may be the true owner of the memory associated
    // with the handler. Consequently, a local copy of the handler is required
    // to ensure that any owning sub-object remains valid until after we have
    // deallocated the memory here.
    detail::binder2<Handler, mxasio::error_code, std::size_t>
      handler(o->handler_, o->ec_, o->bytes_transferred_);
    p.h = mxasio::detail::addressof(handler.handler_);
    p.reset();

    // Make the upcall if required.
    if (owner)
    {
      fenced_block b(fenced_block::half);
      MXASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_, handler.arg2_));
      w.complete(handler, handler.handler_);
      MXASIO_HANDLER_INVOCATION_END;
    }
  }

  static void do_immediate(operation* base, bool, const void* io_ex)
  {
    // Take ownership of the handler object.
    MXASIO_ASSUME(base != 0);
    descriptor_read_op* o(static_cast<descriptor_read_op*>(base));
    ptr p = { mxasio::detail::addressof(o->handler_), o, o };

    MXASIO_HANDLER_COMPLETION((*o));

    // Take ownership of the operation's outstanding work.
    immediate_handler_work<Handler, IoExecutor> w(
        static_cast<handler_work<Handler, IoExecutor>&&>(
          o->work_));

    MXASIO_ERROR_LOCATION(o->ec_);

    // Make a copy of the handler so that the memory can be deallocated before
    // the upcall is made. Even if we're not about to make an upcall, a
    // sub-object of the handler may be the true owner of the memory associated
    // with the handler. Consequently, a local copy of the handler is required
    // to ensure that any owning sub-object remains valid until after we have
    // deallocated the memory here.
    detail::binder2<Handler, mxasio::error_code, std::size_t>
      handler(o->handler_, o->ec_, o->bytes_transferred_);
    p.h = mxasio::detail::addressof(handler.handler_);
    p.reset();

    MXASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_, handler.arg2_));
    w.complete(handler, handler.handler_, io_ex);
    MXASIO_HANDLER_INVOCATION_END;
  }

private:
  Handler handler_;
  handler_work<Handler, IoExecutor> work_;
};

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // !defined(MXASIO_WINDOWS) && !defined(__CYGWIN__)

#endif // MXASIO_DETAIL_DESCRIPTOR_READ_OP_HPP
