//
// ip/address.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_IP_ADDRESS_HPP
#define MXASIO_IP_ADDRESS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include <functional>
#include <string>
#include "mxasio/detail/throw_exception.hpp"
#include "mxasio/detail/string_view.hpp"
#include "mxasio/detail/type_traits.hpp"
#include "mxasio/error_code.hpp"
#include "mxasio/ip/address_v4.hpp"
#include "mxasio/ip/address_v6.hpp"
#include "mxasio/ip/bad_address_cast.hpp"

#if !defined(MXASIO_NO_IOSTREAM)
# include <iosfwd>
#endif // !defined(MXASIO_NO_IOSTREAM)

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace ip {

/// Implements version-independent IP addresses.
/**
 * The mxasio::ip::address class provides the ability to use either IP
 * version 4 or version 6 addresses.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 */
class address
{
public:
  /// Default constructor.
  MXASIO_DECL address() noexcept;

  /// Construct an address from an IPv4 address.
  MXASIO_DECL address(
      const mxasio::ip::address_v4& ipv4_address) noexcept;

  /// Construct an address from an IPv6 address.
  MXASIO_DECL address(
      const mxasio::ip::address_v6& ipv6_address) noexcept;

  /// Copy constructor.
  MXASIO_DECL address(const address& other) noexcept;

  /// Move constructor.
  MXASIO_DECL address(address&& other) noexcept;

  /// Assign from another address.
  MXASIO_DECL address& operator=(const address& other) noexcept;

  /// Move-assign from another address.
  MXASIO_DECL address& operator=(address&& other) noexcept;

  /// Assign from an IPv4 address.
  MXASIO_DECL address& operator=(
      const mxasio::ip::address_v4& ipv4_address) noexcept;

  /// Assign from an IPv6 address.
  MXASIO_DECL address& operator=(
      const mxasio::ip::address_v6& ipv6_address) noexcept;

  /// Get whether the address is an IP version 4 address.
  bool is_v4() const noexcept
  {
    return type_ == ipv4;
  }

  /// Get whether the address is an IP version 6 address.
  bool is_v6() const noexcept
  {
    return type_ == ipv6;
  }

  /// Get the address as an IP version 4 address.
  MXASIO_DECL mxasio::ip::address_v4 to_v4() const;

  /// Get the address as an IP version 6 address.
  MXASIO_DECL mxasio::ip::address_v6 to_v6() const;

  /// Get the address as a string.
  MXASIO_DECL std::string to_string() const;

#if !defined(MXASIO_NO_DEPRECATED)
  /// (Deprecated: Use other overload.) Get the address as a string.
  MXASIO_DECL std::string to_string(mxasio::error_code& ec) const;

  /// (Deprecated: Use make_address().) Create an address from an IPv4 address
  /// string in dotted decimal form, or from an IPv6 address in hexadecimal
  /// notation.
  static address from_string(const char* str);

  /// (Deprecated: Use make_address().) Create an address from an IPv4 address
  /// string in dotted decimal form, or from an IPv6 address in hexadecimal
  /// notation.
  static address from_string(const char* str, mxasio::error_code& ec);

  /// (Deprecated: Use make_address().) Create an address from an IPv4 address
  /// string in dotted decimal form, or from an IPv6 address in hexadecimal
  /// notation.
  static address from_string(const std::string& str);

  /// (Deprecated: Use make_address().) Create an address from an IPv4 address
  /// string in dotted decimal form, or from an IPv6 address in hexadecimal
  /// notation.
  static address from_string(
      const std::string& str, mxasio::error_code& ec);
#endif // !defined(MXASIO_NO_DEPRECATED)

  /// Determine whether the address is a loopback address.
  MXASIO_DECL bool is_loopback() const noexcept;

  /// Determine whether the address is unspecified.
  MXASIO_DECL bool is_unspecified() const noexcept;

  /// Determine whether the address is a multicast address.
  MXASIO_DECL bool is_multicast() const noexcept;

  /// Compare two addresses for equality.
  MXASIO_DECL friend bool operator==(const address& a1,
      const address& a2) noexcept;

  /// Compare two addresses for inequality.
  friend bool operator!=(const address& a1,
      const address& a2) noexcept
  {
    return !(a1 == a2);
  }

  /// Compare addresses for ordering.
  MXASIO_DECL friend bool operator<(const address& a1,
      const address& a2) noexcept;

  /// Compare addresses for ordering.
  friend bool operator>(const address& a1,
      const address& a2) noexcept
  {
    return a2 < a1;
  }

  /// Compare addresses for ordering.
  friend bool operator<=(const address& a1,
      const address& a2) noexcept
  {
    return !(a2 < a1);
  }

  /// Compare addresses for ordering.
  friend bool operator>=(const address& a1,
      const address& a2) noexcept
  {
    return !(a1 < a2);
  }

private:
  // The type of the address.
  enum { ipv4, ipv6 } type_;

  // The underlying IPv4 address.
  mxasio::ip::address_v4 ipv4_address_;

  // The underlying IPv6 address.
  mxasio::ip::address_v6 ipv6_address_;
};

/// Create an address from an IPv4 address string in dotted decimal form,
/// or from an IPv6 address in hexadecimal notation.
/**
 * @relates address
 */
MXASIO_DECL address make_address(const char* str);

/// Create an address from an IPv4 address string in dotted decimal form,
/// or from an IPv6 address in hexadecimal notation.
/**
 * @relates address
 */
MXASIO_DECL address make_address(const char* str,
    mxasio::error_code& ec) noexcept;

/// Create an address from an IPv4 address string in dotted decimal form,
/// or from an IPv6 address in hexadecimal notation.
/**
 * @relates address
 */
MXASIO_DECL address make_address(const std::string& str);

/// Create an address from an IPv4 address string in dotted decimal form,
/// or from an IPv6 address in hexadecimal notation.
/**
 * @relates address
 */
MXASIO_DECL address make_address(const std::string& str,
    mxasio::error_code& ec) noexcept;

#if defined(MXASIO_HAS_STRING_VIEW) \
  || defined(GENERATING_DOCUMENTATION)

/// Create an address from an IPv4 address string in dotted decimal form,
/// or from an IPv6 address in hexadecimal notation.
/**
 * @relates address
 */
MXASIO_DECL address make_address(string_view str);

/// Create an address from an IPv4 address string in dotted decimal form,
/// or from an IPv6 address in hexadecimal notation.
/**
 * @relates address
 */
MXASIO_DECL address make_address(string_view str,
    mxasio::error_code& ec) noexcept;

#endif // defined(MXASIO_HAS_STRING_VIEW)
       //  || defined(GENERATING_DOCUMENTATION)

#if !defined(MXASIO_NO_IOSTREAM)

/// Output an address as a string.
/**
 * Used to output a human-readable string for a specified address.
 *
 * @param os The output stream to which the string will be written.
 *
 * @param addr The address to be written.
 *
 * @return The output stream.
 *
 * @relates mxasio::ip::address
 */
template <typename Elem, typename Traits>
std::basic_ostream<Elem, Traits>& operator<<(
    std::basic_ostream<Elem, Traits>& os, const address& addr);

#endif // !defined(MXASIO_NO_IOSTREAM)

} // namespace ip
} // namespace mxasio

namespace std {

template <>
struct hash<mxasio::ip::address>
{
  std::size_t operator()(const mxasio::ip::address& addr)
    const noexcept
  {
    return addr.is_v4()
      ? std::hash<mxasio::ip::address_v4>()(addr.to_v4())
      : std::hash<mxasio::ip::address_v6>()(addr.to_v6());
  }
};

} // namespace std

#include "mxasio/detail/pop_options.hpp"

#include "mxasio/ip/impl/address.hpp"
#if defined(MXASIO_HEADER_ONLY)
#include "mxasio/ip/impl/address.ipp"
#endif // defined(MXASIO_HEADER_ONLY)

#endif // MXASIO_IP_ADDRESS_HPP
