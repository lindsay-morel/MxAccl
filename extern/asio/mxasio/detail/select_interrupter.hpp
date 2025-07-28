//
// detail/select_interrupter.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_SELECT_INTERRUPTER_HPP
#define MXASIO_DETAIL_SELECT_INTERRUPTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if !defined(MXASIO_WINDOWS_RUNTIME)

#if defined(MXASIO_WINDOWS) || defined(__CYGWIN__) || defined(__SYMBIAN32__)
#include "mxasio/detail/socket_select_interrupter.hpp"
#elif defined(MXASIO_HAS_EVENTFD)
#include "mxasio/detail/eventfd_select_interrupter.hpp"
#else
#include "mxasio/detail/pipe_select_interrupter.hpp"
#endif

namespace mxasio {
namespace detail {

#if defined(MXASIO_WINDOWS) || defined(__CYGWIN__) || defined(__SYMBIAN32__)
typedef socket_select_interrupter select_interrupter;
#elif defined(MXASIO_HAS_EVENTFD)
typedef eventfd_select_interrupter select_interrupter;
#else
typedef pipe_select_interrupter select_interrupter;
#endif

} // namespace detail
} // namespace mxasio

#endif // !defined(MXASIO_WINDOWS_RUNTIME)

#endif // MXASIO_DETAIL_SELECT_INTERRUPTER_HPP
