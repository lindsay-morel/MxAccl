//
// detail/timer_scheduler.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_TIMER_SCHEDULER_HPP
#define MXASIO_DETAIL_TIMER_SCHEDULER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include "mxasio/detail/timer_scheduler_fwd.hpp"

#if defined(MXASIO_WINDOWS_RUNTIME)
#include "mxasio/detail/winrt_timer_scheduler.hpp"
#elif defined(MXASIO_HAS_IOCP)
#include "mxasio/detail/win_iocp_io_context.hpp"
#elif defined(MXASIO_HAS_IO_URING_AS_DEFAULT)
#include "mxasio/detail/io_uring_service.hpp"
#elif defined(MXASIO_HAS_EPOLL)
#include "mxasio/detail/epoll_reactor.hpp"
#elif defined(MXASIO_HAS_KQUEUE)
#include "mxasio/detail/kqueue_reactor.hpp"
#elif defined(MXASIO_HAS_DEV_POLL)
#include "mxasio/detail/dev_poll_reactor.hpp"
#else
#include "mxasio/detail/select_reactor.hpp"
#endif

#endif // MXASIO_DETAIL_TIMER_SCHEDULER_HPP
