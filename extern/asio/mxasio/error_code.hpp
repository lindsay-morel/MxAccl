//
// error_code.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_ERROR_CODE_HPP
#define MXASIO_ERROR_CODE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include <system_error>

#include "mxasio/detail/push_options.hpp"

namespace mxasio {

typedef std::error_category error_category;
typedef std::error_code error_code;

/// Returns the error category used for the system errors produced by asio.
extern MXASIO_DECL const error_category& system_category();

} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#if defined(MXASIO_HEADER_ONLY)
#include "mxasio/impl/error_code.ipp"
#endif // defined(MXASIO_HEADER_ONLY)

#endif // MXASIO_ERROR_CODE_HPP
