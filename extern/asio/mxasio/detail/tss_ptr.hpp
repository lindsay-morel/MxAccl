//
// detail/tss_ptr.hpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_TSS_PTR_HPP
#define MXASIO_DETAIL_TSS_PTR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if !defined(MXASIO_HAS_THREADS)
#include "mxasio/detail/null_tss_ptr.hpp"
#elif defined(MXASIO_HAS_THREAD_KEYWORD_EXTENSION)
#include "mxasio/detail/keyword_tss_ptr.hpp"
#elif defined(MXASIO_WINDOWS)
#include "mxasio/detail/win_tss_ptr.hpp"
#elif defined(MXASIO_HAS_PTHREADS)
#include "mxasio/detail/posix_tss_ptr.hpp"
#else
# error Only Windows and POSIX are supported!
#endif

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

template <typename T>
class tss_ptr
#if !defined(MXASIO_HAS_THREADS)
  : public null_tss_ptr<T>
#elif defined(MXASIO_HAS_THREAD_KEYWORD_EXTENSION)
  : public keyword_tss_ptr<T>
#elif defined(MXASIO_WINDOWS)
  : public win_tss_ptr<T>
#elif defined(MXASIO_HAS_PTHREADS)
  : public posix_tss_ptr<T>
#endif
{
public:
  void operator=(T* value)
  {
#if !defined(MXASIO_HAS_THREADS)
    null_tss_ptr<T>::operator=(value);
#elif defined(MXASIO_HAS_THREAD_KEYWORD_EXTENSION)
    keyword_tss_ptr<T>::operator=(value);
#elif defined(MXASIO_WINDOWS)
    win_tss_ptr<T>::operator=(value);
#elif defined(MXASIO_HAS_PTHREADS)
    posix_tss_ptr<T>::operator=(value);
#endif
  }
};

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // MXASIO_DETAIL_TSS_PTR_HPP
