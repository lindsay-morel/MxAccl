//
// detail/impl/win_tss_ptr.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_IMPL_WIN_TSS_PTR_IPP
#define MXASIO_DETAIL_IMPL_WIN_TSS_PTR_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_WINDOWS)

#include "mxasio/detail/throw_error.hpp"
#include "mxasio/detail/win_tss_ptr.hpp"
#include "mxasio/error.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

DWORD win_tss_ptr_create()
{
#if defined(UNDER_CE)
  const DWORD out_of_indexes = 0xFFFFFFFF;
#else
  const DWORD out_of_indexes = TLS_OUT_OF_INDEXES;
#endif

  DWORD tss_key = ::TlsAlloc();
  if (tss_key == out_of_indexes)
  {
    DWORD last_error = ::GetLastError();
    mxasio::error_code ec(last_error,
        mxasio::error::get_system_category());
    mxasio::detail::throw_error(ec, "tss");
  }
  return tss_key;
}

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // defined(MXASIO_WINDOWS)

#endif // MXASIO_DETAIL_IMPL_WIN_TSS_PTR_IPP
