//
// ip/impl/host_name.ipp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_IP_IMPL_HOST_NAME_IPP
#define MXASIO_IP_IMPL_HOST_NAME_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include "mxasio/detail/socket_ops.hpp"
#include "mxasio/detail/throw_error.hpp"
#include "mxasio/detail/winsock_init.hpp"
#include "mxasio/ip/host_name.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace ip {

std::string host_name()
{
  char name[1024];
  mxasio::error_code ec;
  if (mxasio::detail::socket_ops::gethostname(name, sizeof(name), ec) != 0)
  {
    mxasio::detail::throw_error(ec);
    return std::string();
  }
  return std::string(name);
}

std::string host_name(mxasio::error_code& ec)
{
  char name[1024];
  if (mxasio::detail::socket_ops::gethostname(name, sizeof(name), ec) != 0)
    return std::string();
  return std::string(name);
}

} // namespace ip
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // MXASIO_IP_IMPL_HOST_NAME_IPP
