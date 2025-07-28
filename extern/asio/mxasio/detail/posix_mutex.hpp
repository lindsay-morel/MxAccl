//
// detail/posix_mutex.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_POSIX_MUTEX_HPP
#define MXASIO_DETAIL_POSIX_MUTEX_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_HAS_PTHREADS)

#include <pthread.h>
#include "mxasio/detail/noncopyable.hpp"
#include "mxasio/detail/scoped_lock.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

class posix_event;

class posix_mutex
  : private noncopyable
{
public:
  typedef mxasio::detail::scoped_lock<posix_mutex> scoped_lock;

  // Constructor.
  MXASIO_DECL posix_mutex();

  // Destructor.
  ~posix_mutex()
  {
    ::pthread_mutex_destroy(&mutex_); // Ignore EBUSY.
  }

  // Lock the mutex.
  void lock()
  {
    (void)::pthread_mutex_lock(&mutex_); // Ignore EINVAL.
  }

  // Unlock the mutex.
  void unlock()
  {
    (void)::pthread_mutex_unlock(&mutex_); // Ignore EINVAL.
  }

private:
  friend class posix_event;
  ::pthread_mutex_t mutex_;
};

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#if defined(MXASIO_HEADER_ONLY)
#include "mxasio/detail/impl/posix_mutex.ipp"
#endif // defined(MXASIO_HEADER_ONLY)

#endif // defined(MXASIO_HAS_PTHREADS)

#endif // MXASIO_DETAIL_POSIX_MUTEX_HPP
