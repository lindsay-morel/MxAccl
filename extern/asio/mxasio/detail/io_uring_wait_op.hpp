//
// detail/io_uring_wait_op.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_IO_URING_WAIT_OP_HPP
#define MXASIO_DETAIL_IO_URING_WAIT_OP_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include "mxasio/detail/bind_handler.hpp"
#include "mxasio/detail/fenced_block.hpp"
#include "mxasio/detail/handler_alloc_helpers.hpp"
#include "mxasio/detail/handler_work.hpp"
#include "mxasio/detail/io_uring_operation.hpp"
#include "mxasio/detail/memory.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

template <typename Handler, typename IoExecutor>
class io_uring_wait_op : public io_uring_operation
{
public:
  MXASIO_DEFINE_HANDLER_PTR(io_uring_wait_op);

  io_uring_wait_op(const mxasio::error_code& success_ec, int descriptor,
      int poll_flags, Handler& handler, const IoExecutor& io_ex)
    : io_uring_operation(success_ec, &io_uring_wait_op::do_prepare,
        &io_uring_wait_op::do_perform, &io_uring_wait_op::do_complete),
      handler_(static_cast<Handler&&>(handler)),
      work_(handler_, io_ex),
      descriptor_(descriptor),
      poll_flags_(poll_flags)
  {
  }

  static void do_prepare(io_uring_operation* base, ::io_uring_sqe* sqe)
  {
    MXASIO_ASSUME(base != 0);
    io_uring_wait_op* o(static_cast<io_uring_wait_op*>(base));

    ::io_uring_prep_poll_add(sqe, o->descriptor_, o->poll_flags_);
  }

  static bool do_perform(io_uring_operation*, bool after_completion)
  {
    return after_completion;
  }

  static void do_complete(void* owner, operation* base,
      const mxasio::error_code& /*ec*/,
      std::size_t /*bytes_transferred*/)
  {
    // Take ownership of the handler object.
    MXASIO_ASSUME(base != 0);
    io_uring_wait_op* o(static_cast<io_uring_wait_op*>(base));
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
    detail::binder1<Handler, mxasio::error_code>
      handler(o->handler_, o->ec_);
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

private:
  Handler handler_;
  handler_work<Handler, IoExecutor> work_;
  int descriptor_;
  int poll_flags_;
};

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // MXASIO_DETAIL_IO_URING_WAIT_OP_HPP
