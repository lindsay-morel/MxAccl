//
// detail/impl/throw_error.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_IMPL_THROW_ERROR_IPP
#define MXASIO_DETAIL_IMPL_THROW_ERROR_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include "mxasio/detail/throw_error.hpp"
#include "mxasio/system_error.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

void do_throw_error(
    const mxasio::error_code& err
    MXASIO_SOURCE_LOCATION_PARAM)
{
  mxasio::system_error e(err);
  mxasio::detail::throw_exception(e MXASIO_SOURCE_LOCATION_ARG);
}

void do_throw_error(
    const mxasio::error_code& err,
    const char* location
    MXASIO_SOURCE_LOCATION_PARAM)
{
  mxasio::system_error e(err, location);
  mxasio::detail::throw_exception(e MXASIO_SOURCE_LOCATION_ARG);
}

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // MXASIO_DETAIL_IMPL_THROW_ERROR_IPP
