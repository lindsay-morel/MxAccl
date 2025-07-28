//
// detail/thread.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_THREAD_HPP
#define MXASIO_DETAIL_THREAD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if !defined(MXASIO_HAS_THREADS)
#include "mxasio/detail/null_thread.hpp"
#elif defined(MXASIO_HAS_PTHREADS)
#include "mxasio/detail/posix_thread.hpp"
#elif defined(MXASIO_WINDOWS)
# if defined(UNDER_CE)
#include "mxasio/detail/wince_thread.hpp"
# elif defined(MXASIO_WINDOWS_APP)
#include "mxasio/detail/winapp_thread.hpp"
# else
#include "mxasio/detail/win_thread.hpp"
# endif
#else
#include "mxasio/detail/std_thread.hpp"
#endif

namespace mxasio {
namespace detail {

#if !defined(MXASIO_HAS_THREADS)
typedef null_thread thread;
#elif defined(MXASIO_HAS_PTHREADS)
typedef posix_thread thread;
#elif defined(MXASIO_WINDOWS)
# if defined(UNDER_CE)
typedef wince_thread thread;
# elif defined(MXASIO_WINDOWS_APP)
typedef winapp_thread thread;
# else
typedef win_thread thread;
# endif
#else
typedef std_thread thread;
#endif

} // namespace detail
} // namespace mxasio

#endif // MXASIO_DETAIL_THREAD_HPP
