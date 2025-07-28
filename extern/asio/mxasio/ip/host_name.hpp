//
// ip/host_name.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_IP_HOST_NAME_HPP
#define MXASIO_IP_HOST_NAME_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include <string>
#include "mxasio/error_code.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace ip {

/// Get the current host name.
MXASIO_DECL std::string host_name();

/// Get the current host name.
MXASIO_DECL std::string host_name(mxasio::error_code& ec);

} // namespace ip
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#if defined(MXASIO_HEADER_ONLY)
#include "mxasio/ip/impl/host_name.ipp"
#endif // defined(MXASIO_HEADER_ONLY)

#endif // MXASIO_IP_HOST_NAME_HPP
