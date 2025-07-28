//
// local/detail/endpoint.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Derived from a public domain implementation written by Daniel Casimiro.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_LOCAL_DETAIL_ENDPOINT_HPP
#define MXASIO_LOCAL_DETAIL_ENDPOINT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_HAS_LOCAL_SOCKETS)

#include <cstddef>
#include <string>
#include "mxasio/detail/socket_types.hpp"
#include "mxasio/detail/string_view.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace local {
namespace detail {

// Helper class for implementing a UNIX domain endpoint.
class endpoint
{
public:
  // Default constructor.
  MXASIO_DECL endpoint();

  // Construct an endpoint using the specified path name.
  MXASIO_DECL endpoint(const char* path_name);

  // Construct an endpoint using the specified path name.
  MXASIO_DECL endpoint(const std::string& path_name);

  #if defined(MXASIO_HAS_STRING_VIEW)
  // Construct an endpoint using the specified path name.
  MXASIO_DECL endpoint(string_view path_name);
  #endif // defined(MXASIO_HAS_STRING_VIEW)

  // Copy constructor.
  endpoint(const endpoint& other)
    : data_(other.data_),
      path_length_(other.path_length_)
  {
  }

  // Assign from another endpoint.
  endpoint& operator=(const endpoint& other)
  {
    data_ = other.data_;
    path_length_ = other.path_length_;
    return *this;
  }

  // Get the underlying endpoint in the native type.
  mxasio::detail::socket_addr_type* data()
  {
    return &data_.base;
  }

  // Get the underlying endpoint in the native type.
  const mxasio::detail::socket_addr_type* data() const
  {
    return &data_.base;
  }

  // Get the underlying size of the endpoint in the native type.
  std::size_t size() const
  {
    return path_length_
      + offsetof(mxasio::detail::sockaddr_un_type, sun_path);
  }

  // Set the underlying size of the endpoint in the native type.
  MXASIO_DECL void resize(std::size_t size);

  // Get the capacity of the endpoint in the native type.
  std::size_t capacity() const
  {
    return sizeof(mxasio::detail::sockaddr_un_type);
  }

  // Get the path associated with the endpoint.
  MXASIO_DECL std::string path() const;

  // Set the path associated with the endpoint.
  MXASIO_DECL void path(const char* p);

  // Set the path associated with the endpoint.
  MXASIO_DECL void path(const std::string& p);

  // Compare two endpoints for equality.
  MXASIO_DECL friend bool operator==(
      const endpoint& e1, const endpoint& e2);

  // Compare endpoints for ordering.
  MXASIO_DECL friend bool operator<(
      const endpoint& e1, const endpoint& e2);

private:
  // The underlying UNIX socket address.
  union data_union
  {
    mxasio::detail::socket_addr_type base;
    mxasio::detail::sockaddr_un_type local;
  } data_;

  // The length of the path associated with the endpoint.
  std::size_t path_length_;

  // Initialise with a specified path.
  MXASIO_DECL void init(const char* path, std::size_t path_length);
};

} // namespace detail
} // namespace local
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#if defined(MXASIO_HEADER_ONLY)
#include "mxasio/local/detail/impl/endpoint.ipp"
#endif // defined(MXASIO_HEADER_ONLY)

#endif // defined(MXASIO_HAS_LOCAL_SOCKETS)

#endif // MXASIO_LOCAL_DETAIL_ENDPOINT_HPP
