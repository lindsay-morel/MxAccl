//
// detail/blocking_executor_op.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_BLOCKING_EXECUTOR_OP_HPP
#define MXASIO_DETAIL_BLOCKING_EXECUTOR_OP_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include "mxasio/detail/event.hpp"
#include "mxasio/detail/fenced_block.hpp"
#include "mxasio/detail/mutex.hpp"
#include "mxasio/detail/scheduler_operation.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

template <typename Operation = scheduler_operation>
class blocking_executor_op_base : public Operation
{
public:
  blocking_executor_op_base(typename Operation::func_type complete_func)
    : Operation(complete_func),
      is_complete_(false)
  {
  }

  void wait()
  {
    mxasio::detail::mutex::scoped_lock lock(mutex_);
    while (!is_complete_)
      event_.wait(lock);
  }

protected:
  struct do_complete_cleanup
  {
    ~do_complete_cleanup()
    {
      mxasio::detail::mutex::scoped_lock lock(op_->mutex_);
      op_->is_complete_ = true;
      op_->event_.unlock_and_signal_one_for_destruction(lock);
    }

    blocking_executor_op_base* op_;
  };

private:
  mxasio::detail::mutex mutex_;
  mxasio::detail::event event_;
  bool is_complete_;
};

template <typename Handler, typename Operation = scheduler_operation>
class blocking_executor_op : public blocking_executor_op_base<Operation>
{
public:
  blocking_executor_op(Handler& h)
    : blocking_executor_op_base<Operation>(&blocking_executor_op::do_complete),
      handler_(h)
  {
  }

  static void do_complete(void* owner, Operation* base,
      const mxasio::error_code& /*ec*/,
      std::size_t /*bytes_transferred*/)
  {
    MXASIO_ASSUME(base != 0);
    blocking_executor_op* o(static_cast<blocking_executor_op*>(base));

    typename blocking_executor_op_base<Operation>::do_complete_cleanup
      on_exit = { o };
    (void)on_exit;

    MXASIO_HANDLER_COMPLETION((*o));

    // Make the upcall if required.
    if (owner)
    {
      fenced_block b(fenced_block::half);
      MXASIO_HANDLER_INVOCATION_BEGIN(());
      static_cast<Handler&&>(o->handler_)();
      MXASIO_HANDLER_INVOCATION_END;
    }
  }

private:
  Handler& handler_;
};

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // MXASIO_DETAIL_BLOCKING_EXECUTOR_OP_HPP
