//
// detail/throw_error.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_THROW_ERROR_HPP
#define MXASIO_DETAIL_THROW_ERROR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include "mxasio/detail/throw_exception.hpp"
#include "mxasio/error_code.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

MXASIO_DECL void do_throw_error(
    const mxasio::error_code& err
    MXASIO_SOURCE_LOCATION_PARAM);

MXASIO_DECL void do_throw_error(
    const mxasio::error_code& err,
    const char* location
    MXASIO_SOURCE_LOCATION_PARAM);

inline void throw_error(
    const mxasio::error_code& err
    MXASIO_SOURCE_LOCATION_DEFAULTED_PARAM)
{
  if (err)
    do_throw_error(err MXASIO_SOURCE_LOCATION_ARG);
}

inline void throw_error(
    const mxasio::error_code& err,
    const char* location
    MXASIO_SOURCE_LOCATION_DEFAULTED_PARAM)
{
  if (err)
    do_throw_error(err, location MXASIO_SOURCE_LOCATION_ARG);
}

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#if defined(MXASIO_HEADER_ONLY)
#include "mxasio/detail/impl/throw_error.ipp"
#endif // defined(MXASIO_HEADER_ONLY)

#endif // MXASIO_DETAIL_THROW_ERROR_HPP
