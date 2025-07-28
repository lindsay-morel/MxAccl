//
// ip/detail/endpoint.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_IP_DETAIL_ENDPOINT_HPP
#define MXASIO_IP_DETAIL_ENDPOINT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include <string>
#include "mxasio/detail/socket_types.hpp"
#include "mxasio/detail/winsock_init.hpp"
#include "mxasio/error_code.hpp"
#include "mxasio/ip/address.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace ip {
namespace detail {

// Helper class for implementating an IP endpoint.
class endpoint
{
public:
  // Default constructor.
  MXASIO_DECL endpoint() noexcept;

  // Construct an endpoint using a family and port number.
  MXASIO_DECL endpoint(int family,
      unsigned short port_num) noexcept;

  // Construct an endpoint using an address and port number.
  MXASIO_DECL endpoint(const mxasio::ip::address& addr,
      unsigned short port_num) noexcept;

  // Copy constructor.
  endpoint(const endpoint& other) noexcept
    : data_(other.data_)
  {
  }

  // Assign from another endpoint.
  endpoint& operator=(const endpoint& other) noexcept
  {
    data_ = other.data_;
    return *this;
  }

  // Get the underlying endpoint in the native type.
  mxasio::detail::socket_addr_type* data() noexcept
  {
    return &data_.base;
  }

  // Get the underlying endpoint in the native type.
  const mxasio::detail::socket_addr_type* data() const noexcept
  {
    return &data_.base;
  }

  // Get the underlying size of the endpoint in the native type.
  std::size_t size() const noexcept
  {
    if (is_v4())
      return sizeof(mxasio::detail::sockaddr_in4_type);
    else
      return sizeof(mxasio::detail::sockaddr_in6_type);
  }

  // Set the underlying size of the endpoint in the native type.
  MXASIO_DECL void resize(std::size_t new_size);

  // Get the capacity of the endpoint in the native type.
  std::size_t capacity() const noexcept
  {
    return sizeof(data_);
  }

  // Get the port associated with the endpoint.
  MXASIO_DECL unsigned short port() const noexcept;

  // Set the port associated with the endpoint.
  MXASIO_DECL void port(unsigned short port_num) noexcept;

  // Get the IP address associated with the endpoint.
  MXASIO_DECL mxasio::ip::address address() const noexcept;

  // Set the IP address associated with the endpoint.
  MXASIO_DECL void address(
      const mxasio::ip::address& addr) noexcept;

  // Compare two endpoints for equality.
  MXASIO_DECL friend bool operator==(const endpoint& e1,
      const endpoint& e2) noexcept;

  // Compare endpoints for ordering.
  MXASIO_DECL friend bool operator<(const endpoint& e1,
      const endpoint& e2) noexcept;

  // Determine whether the endpoint is IPv4.
  bool is_v4() const noexcept
  {
    return data_.base.sa_family == MXASIO_OS_DEF(AF_INET);
  }

#if !defined(MXASIO_NO_IOSTREAM)
  // Convert to a string.
  MXASIO_DECL std::string to_string() const;
#endif // !defined(MXASIO_NO_IOSTREAM)

private:
  // The underlying IP socket address.
  union data_union
  {
    mxasio::detail::socket_addr_type base;
    mxasio::detail::sockaddr_in4_type v4;
    mxasio::detail::sockaddr_in6_type v6;
  } data_;
};

} // namespace detail
} // namespace ip
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#if defined(MXASIO_HEADER_ONLY)
#include "mxasio/ip/detail/impl/endpoint.ipp"
#endif // defined(MXASIO_HEADER_ONLY)

#endif // MXASIO_IP_DETAIL_ENDPOINT_HPP
