//
// detail/impl/posix_tss_ptr.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_IMPL_POSIX_TSS_PTR_IPP
#define MXASIO_DETAIL_IMPL_POSIX_TSS_PTR_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_HAS_PTHREADS)

#include "mxasio/detail/posix_tss_ptr.hpp"
#include "mxasio/detail/throw_error.hpp"
#include "mxasio/error.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

void posix_tss_ptr_create(pthread_key_t& key)
{
  int error = ::pthread_key_create(&key, 0);
  mxasio::error_code ec(error,
      mxasio::error::get_system_category());
  mxasio::detail::throw_error(ec, "tss");
}

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // defined(MXASIO_HAS_PTHREADS)

#endif // MXASIO_DETAIL_IMPL_POSIX_TSS_PTR_IPP
