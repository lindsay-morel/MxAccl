//
// detail/event.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_EVENT_HPP
#define MXASIO_DETAIL_EVENT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if !defined(MXASIO_HAS_THREADS)
#include "mxasio/detail/null_event.hpp"
#elif defined(MXASIO_WINDOWS)
#include "mxasio/detail/win_event.hpp"
#elif defined(MXASIO_HAS_PTHREADS)
#include "mxasio/detail/posix_event.hpp"
#else
#include "mxasio/detail/std_event.hpp"
#endif

namespace mxasio {
namespace detail {

#if !defined(MXASIO_HAS_THREADS)
typedef null_event event;
#elif defined(MXASIO_WINDOWS)
typedef win_event event;
#elif defined(MXASIO_HAS_PTHREADS)
typedef posix_event event;
#else
typedef std_event event;
#endif

} // namespace detail
} // namespace mxasio

#endif // MXASIO_DETAIL_EVENT_HPP
