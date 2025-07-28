//
// detail/static_mutex.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_STATIC_MUTEX_HPP
#define MXASIO_DETAIL_STATIC_MUTEX_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if !defined(MXASIO_HAS_THREADS)
#include "mxasio/detail/null_static_mutex.hpp"
#elif defined(MXASIO_WINDOWS)
#include "mxasio/detail/win_static_mutex.hpp"
#elif defined(MXASIO_HAS_PTHREADS)
#include "mxasio/detail/posix_static_mutex.hpp"
#else
#include "mxasio/detail/std_static_mutex.hpp"
#endif

namespace mxasio {
namespace detail {

#if !defined(MXASIO_HAS_THREADS)
typedef null_static_mutex static_mutex;
# define MXASIO_STATIC_MUTEX_INIT MXASIO_NULL_STATIC_MUTEX_INIT
#elif defined(MXASIO_WINDOWS)
typedef win_static_mutex static_mutex;
# define MXASIO_STATIC_MUTEX_INIT MXASIO_WIN_STATIC_MUTEX_INIT
#elif defined(MXASIO_HAS_PTHREADS)
typedef posix_static_mutex static_mutex;
# define MXASIO_STATIC_MUTEX_INIT MXASIO_POSIX_STATIC_MUTEX_INIT
#else
typedef std_static_mutex static_mutex;
# define MXASIO_STATIC_MUTEX_INIT MXASIO_STD_STATIC_MUTEX_INIT
#endif

} // namespace detail
} // namespace mxasio

#endif // MXASIO_DETAIL_STATIC_MUTEX_HPP
