//
// ip/v6_only.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_IP_V6_ONLY_HPP
#define MXASIO_IP_V6_ONLY_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include "mxasio/detail/socket_option.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace ip {

/// Socket option for determining whether an IPv6 socket supports IPv6
/// communication only.
/**
 * Implements the IPPROTO_IPV6/IPV6_V6ONLY socket option.
 *
 * @par Examples
 * Setting the option:
 * @code
 * mxasio::ip::tcp::socket socket(my_context);
 * ...
 * mxasio::ip::v6_only option(true);
 * socket.set_option(option);
 * @endcode
 *
 * @par
 * Getting the current option value:
 * @code
 * mxasio::ip::tcp::socket socket(my_context);
 * ...
 * mxasio::ip::v6_only option;
 * socket.get_option(option);
 * bool v6_only = option.value();
 * @endcode
 *
 * @par Concepts:
 * GettableSocketOption, SettableSocketOption.
 */
#if defined(GENERATING_DOCUMENTATION)
typedef implementation_defined v6_only;
#elif defined(IPV6_V6ONLY)
typedef mxasio::detail::socket_option::boolean<
    IPPROTO_IPV6, IPV6_V6ONLY> v6_only;
#else
typedef mxasio::detail::socket_option::boolean<
    mxasio::detail::custom_socket_option_level,
    mxasio::detail::always_fail_option> v6_only;
#endif

} // namespace ip
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // MXASIO_IP_V6_ONLY_HPP
