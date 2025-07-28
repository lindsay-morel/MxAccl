//
// detail/reactor.hpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_REACTOR_HPP
#define MXASIO_DETAIL_REACTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_HAS_IOCP) || defined(MXASIO_WINDOWS_RUNTIME)
#include "mxasio/detail/null_reactor.hpp"
#elif defined(MXASIO_HAS_IO_URING_AS_DEFAULT)
#include "mxasio/detail/null_reactor.hpp"
#elif defined(MXASIO_HAS_EPOLL)
#include "mxasio/detail/epoll_reactor.hpp"
#elif defined(MXASIO_HAS_KQUEUE)
#include "mxasio/detail/kqueue_reactor.hpp"
#elif defined(MXASIO_HAS_DEV_POLL)
#include "mxasio/detail/dev_poll_reactor.hpp"
#else
#include "mxasio/detail/select_reactor.hpp"
#endif

namespace mxasio {
namespace detail {

#if defined(MXASIO_HAS_IOCP) || defined(MXASIO_WINDOWS_RUNTIME)
typedef null_reactor reactor;
#elif defined(MXASIO_HAS_IO_URING_AS_DEFAULT)
typedef null_reactor reactor;
#elif defined(MXASIO_HAS_EPOLL)
typedef epoll_reactor reactor;
#elif defined(MXASIO_HAS_KQUEUE)
typedef kqueue_reactor reactor;
#elif defined(MXASIO_HAS_DEV_POLL)
typedef dev_poll_reactor reactor;
#else
typedef select_reactor reactor;
#endif

} // namespace detail
} // namespace mxasio

#endif // MXASIO_DETAIL_REACTOR_HPP
