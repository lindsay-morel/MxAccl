//
// ip/impl/address.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_IP_IMPL_ADDRESS_HPP
#define MXASIO_IP_IMPL_ADDRESS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#if !defined(MXASIO_NO_IOSTREAM)

#include "mxasio/detail/throw_error.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace ip {

#if !defined(MXASIO_NO_DEPRECATED)

inline address address::from_string(const char* str)
{
  return mxasio::ip::make_address(str);
}

inline address address::from_string(
    const char* str, mxasio::error_code& ec)
{
  return mxasio::ip::make_address(str, ec);
}

inline address address::from_string(const std::string& str)
{
  return mxasio::ip::make_address(str);
}

inline address address::from_string(
    const std::string& str, mxasio::error_code& ec)
{
  return mxasio::ip::make_address(str, ec);
}

#endif // !defined(MXASIO_NO_DEPRECATED)

template <typename Elem, typename Traits>
std::basic_ostream<Elem, Traits>& operator<<(
    std::basic_ostream<Elem, Traits>& os, const address& addr)
{
  return os << addr.to_string().c_str();
}

} // namespace ip
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // !defined(MXASIO_NO_IOSTREAM)

#endif // MXASIO_IP_IMPL_ADDRESS_HPP
