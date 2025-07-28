//
// detail/fd_set_adapter.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_FD_SET_ADAPTER_HPP
#define MXASIO_DETAIL_FD_SET_ADAPTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if !defined(MXASIO_WINDOWS_RUNTIME)

#include "mxasio/detail/posix_fd_set_adapter.hpp"
#include "mxasio/detail/win_fd_set_adapter.hpp"

namespace mxasio {
namespace detail {

#if defined(MXASIO_WINDOWS) || defined(__CYGWIN__)
typedef win_fd_set_adapter fd_set_adapter;
#else
typedef posix_fd_set_adapter fd_set_adapter;
#endif

} // namespace detail
} // namespace mxasio

#endif // !defined(MXASIO_WINDOWS_RUNTIME)

#endif // MXASIO_DETAIL_FD_SET_ADAPTER_HPP
