//
// detail/timer_scheduler_fwd.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_TIMER_SCHEDULER_FWD_HPP
#define MXASIO_DETAIL_TIMER_SCHEDULER_FWD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

namespace mxasio {
namespace detail {

#if defined(MXASIO_WINDOWS_RUNTIME)
typedef class winrt_timer_scheduler timer_scheduler;
#elif defined(MXASIO_HAS_IOCP)
typedef class win_iocp_io_context timer_scheduler;
#elif defined(MXASIO_HAS_IO_URING_AS_DEFAULT)
typedef class io_uring_service timer_scheduler;
#elif defined(MXASIO_HAS_EPOLL)
typedef class epoll_reactor timer_scheduler;
#elif defined(MXASIO_HAS_KQUEUE)
typedef class kqueue_reactor timer_scheduler;
#elif defined(MXASIO_HAS_DEV_POLL)
typedef class dev_poll_reactor timer_scheduler;
#else
typedef class select_reactor timer_scheduler;
#endif

} // namespace detail
} // namespace mxasio

#endif // MXASIO_DETAIL_TIMER_SCHEDULER_FWD_HPP
