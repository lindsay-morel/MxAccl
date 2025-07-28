//
// ts/netfwd.hpp
// ~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_TS_NETFWD_HPP
#define MXASIO_TS_NETFWD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include "mxasio/detail/chrono.hpp"

#if defined(MXASIO_HAS_BOOST_DATE_TIME)
#include "mxasio/detail/date_time_fwd.hpp"
#endif // defined(MXASIO_HAS_BOOST_DATE_TIME)

#if !defined(MXASIO_USE_TS_EXECUTOR_AS_DEFAULT)
#include "mxasio/execution/blocking.hpp"
#include "mxasio/execution/outstanding_work.hpp"
#include "mxasio/execution/relationship.hpp"
#endif // !defined(MXASIO_USE_TS_EXECUTOR_AS_DEFAULT)

#if !defined(GENERATING_DOCUMENTATION)

#include "mxasio/detail/push_options.hpp"

namespace mxasio {

class execution_context;

template <typename T, typename Executor>
class executor_binder;

#if !defined(MXASIO_EXECUTOR_WORK_GUARD_DECL)
#define MXASIO_EXECUTOR_WORK_GUARD_DECL

template <typename Executor, typename = void, typename = void>
class executor_work_guard;

#endif // !defined(MXASIO_EXECUTOR_WORK_GUARD_DECL)

template <typename Blocking, typename Relationship, typename Allocator>
class basic_system_executor;

#if defined(MXASIO_USE_TS_EXECUTOR_AS_DEFAULT)

class executor;

typedef executor any_io_executor;

#else // defined(MXASIO_USE_TS_EXECUTOR_AS_DEFAULT)

namespace execution {

#if !defined(MXASIO_EXECUTION_ANY_EXECUTOR_FWD_DECL)
#define MXASIO_EXECUTION_ANY_EXECUTOR_FWD_DECL

template <typename... SupportableProperties>
class any_executor;

#endif // !defined(MXASIO_EXECUTION_ANY_EXECUTOR_FWD_DECL)

template <typename U>
struct context_as_t;

template <typename Property>
struct prefer_only;

} // namespace execution

class any_io_executor;

#endif // defined(MXASIO_USE_TS_EXECUTOR_AS_DEFAULT)

template <typename Executor>
class strand;

class io_context;

template <typename Clock>
struct wait_traits;

#if defined(MXASIO_HAS_BOOST_DATE_TIME)

template <typename Time>
struct time_traits;

#endif // defined(MXASIO_HAS_BOOST_DATE_TIME)

#if !defined(MXASIO_BASIC_WAITABLE_TIMER_FWD_DECL)
#define MXASIO_BASIC_WAITABLE_TIMER_FWD_DECL

template <typename Clock,
    typename WaitTraits = wait_traits<Clock>,
    typename Executor = any_io_executor>
class basic_waitable_timer;

#endif // !defined(MXASIO_BASIC_WAITABLE_TIMER_FWD_DECL)

typedef basic_waitable_timer<chrono::system_clock> system_timer;

typedef basic_waitable_timer<chrono::steady_clock> steady_timer;

typedef basic_waitable_timer<chrono::high_resolution_clock>
  high_resolution_timer;

#if !defined(MXASIO_BASIC_SOCKET_FWD_DECL)
#define MXASIO_BASIC_SOCKET_FWD_DECL

template <typename Protocol, typename Executor = any_io_executor>
class basic_socket;

#endif // !defined(MXASIO_BASIC_SOCKET_FWD_DECL)

#if !defined(MXASIO_BASIC_DATAGRAM_SOCKET_FWD_DECL)
#define MXASIO_BASIC_DATAGRAM_SOCKET_FWD_DECL

template <typename Protocol, typename Executor = any_io_executor>
class basic_datagram_socket;

#endif // !defined(MXASIO_BASIC_DATAGRAM_SOCKET_FWD_DECL)

#if !defined(MXASIO_BASIC_STREAM_SOCKET_FWD_DECL)
#define MXASIO_BASIC_STREAM_SOCKET_FWD_DECL

// Forward declaration with defaulted arguments.
template <typename Protocol, typename Executor = any_io_executor>
class basic_stream_socket;

#endif // !defined(MXASIO_BASIC_STREAM_SOCKET_FWD_DECL)

#if !defined(MXASIO_BASIC_SOCKET_ACCEPTOR_FWD_DECL)
#define MXASIO_BASIC_SOCKET_ACCEPTOR_FWD_DECL

template <typename Protocol, typename Executor = any_io_executor>
class basic_socket_acceptor;

#endif // !defined(MXASIO_BASIC_SOCKET_ACCEPTOR_FWD_DECL)

#if !defined(MXASIO_BASIC_SOCKET_STREAMBUF_FWD_DECL)
#define MXASIO_BASIC_SOCKET_STREAMBUF_FWD_DECL

// Forward declaration with defaulted arguments.
template <typename Protocol,
#if defined(MXASIO_HAS_BOOST_DATE_TIME) \
  || defined(GENERATING_DOCUMENTATION)
    typename Clock = boost::posix_time::ptime,
    typename WaitTraits = time_traits<Clock>>
#else
    typename Clock = chrono::steady_clock,
    typename WaitTraits = wait_traits<Clock>>
#endif
class basic_socket_streambuf;

#endif // !defined(MXASIO_BASIC_SOCKET_STREAMBUF_FWD_DECL)

#if !defined(MXASIO_BASIC_SOCKET_IOSTREAM_FWD_DECL)
#define MXASIO_BASIC_SOCKET_IOSTREAM_FWD_DECL

// Forward declaration with defaulted arguments.
template <typename Protocol,
#if defined(MXASIO_HAS_BOOST_DATE_TIME) \
  || defined(GENERATING_DOCUMENTATION)
    typename Clock = boost::posix_time::ptime,
    typename WaitTraits = time_traits<Clock>>
#else
    typename Clock = chrono::steady_clock,
    typename WaitTraits = wait_traits<Clock>>
#endif
class basic_socket_iostream;

#endif // !defined(MXASIO_BASIC_SOCKET_IOSTREAM_FWD_DECL)

namespace ip {

class address;

class address_v4;

class address_v6;

template <typename Address>
class basic_address_iterator;

typedef basic_address_iterator<address_v4> address_v4_iterator;

typedef basic_address_iterator<address_v6> address_v6_iterator;

template <typename Address>
class basic_address_range;

typedef basic_address_range<address_v4> address_v4_range;

typedef basic_address_range<address_v6> address_v6_range;

class network_v4;

class network_v6;

template <typename InternetProtocol>
class basic_endpoint;

template <typename InternetProtocol>
class basic_resolver_entry;

template <typename InternetProtocol>
class basic_resolver_results;

#if !defined(MXASIO_IP_BASIC_RESOLVER_FWD_DECL)
#define MXASIO_IP_BASIC_RESOLVER_FWD_DECL

template <typename InternetProtocol, typename Executor = any_io_executor>
class basic_resolver;

#endif // !defined(MXASIO_IP_BASIC_RESOLVER_FWD_DECL)

class tcp;

class udp;

} // namespace ip
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // !defined(GENERATING_DOCUMENTATION)

#endif // MXASIO_TS_NETFWD_HPP
