//
// detail/global.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_GLOBAL_HPP
#define MXASIO_DETAIL_GLOBAL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if !defined(MXASIO_HAS_THREADS)
#include "mxasio/detail/null_global.hpp"
#elif defined(MXASIO_WINDOWS)
#include "mxasio/detail/win_global.hpp"
#elif defined(MXASIO_HAS_PTHREADS)
#include "mxasio/detail/posix_global.hpp"
#else
#include "mxasio/detail/std_global.hpp"
#endif

namespace mxasio {
namespace detail {

template <typename T>
inline T& global()
{
#if !defined(MXASIO_HAS_THREADS)
  return null_global<T>();
#elif defined(MXASIO_WINDOWS)
  return win_global<T>();
#elif defined(MXASIO_HAS_PTHREADS)
  return posix_global<T>();
#else
  return std_global<T>();
#endif
}

} // namespace detail
} // namespace mxasio

#endif // MXASIO_DETAIL_GLOBAL_HPP
