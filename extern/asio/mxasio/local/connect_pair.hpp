//
// local/connect_pair.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_LOCAL_CONNECT_PAIR_HPP
#define MXASIO_LOCAL_CONNECT_PAIR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_HAS_LOCAL_SOCKETS) \
  || defined(GENERATING_DOCUMENTATION)

#include "mxasio/basic_socket.hpp"
#include "mxasio/detail/socket_ops.hpp"
#include "mxasio/detail/throw_error.hpp"
#include "mxasio/error.hpp"
#include "mxasio/local/basic_endpoint.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace local {

/// Create a pair of connected sockets.
template <typename Protocol, typename Executor1, typename Executor2>
void connect_pair(basic_socket<Protocol, Executor1>& socket1,
    basic_socket<Protocol, Executor2>& socket2);

/// Create a pair of connected sockets.
template <typename Protocol, typename Executor1, typename Executor2>
MXASIO_SYNC_OP_VOID connect_pair(basic_socket<Protocol, Executor1>& socket1,
    basic_socket<Protocol, Executor2>& socket2, mxasio::error_code& ec);

template <typename Protocol, typename Executor1, typename Executor2>
inline void connect_pair(basic_socket<Protocol, Executor1>& socket1,
    basic_socket<Protocol, Executor2>& socket2)
{
  mxasio::error_code ec;
  connect_pair(socket1, socket2, ec);
  mxasio::detail::throw_error(ec, "connect_pair");
}

template <typename Protocol, typename Executor1, typename Executor2>
inline MXASIO_SYNC_OP_VOID connect_pair(
    basic_socket<Protocol, Executor1>& socket1,
    basic_socket<Protocol, Executor2>& socket2, mxasio::error_code& ec)
{
  // Check that this function is only being used with a UNIX domain socket.
  mxasio::local::basic_endpoint<Protocol>* tmp
    = static_cast<typename Protocol::endpoint*>(0);
  (void)tmp;

  Protocol protocol;
  mxasio::detail::socket_type sv[2];
  if (mxasio::detail::socket_ops::socketpair(protocol.family(),
        protocol.type(), protocol.protocol(), sv, ec)
      == mxasio::detail::socket_error_retval)
    MXASIO_SYNC_OP_VOID_RETURN(ec);

  socket1.assign(protocol, sv[0], ec);
  if (ec)
  {
    mxasio::error_code temp_ec;
    mxasio::detail::socket_ops::state_type state[2] = { 0, 0 };
    mxasio::detail::socket_ops::close(sv[0], state[0], true, temp_ec);
    mxasio::detail::socket_ops::close(sv[1], state[1], true, temp_ec);
    MXASIO_SYNC_OP_VOID_RETURN(ec);
  }

  socket2.assign(protocol, sv[1], ec);
  if (ec)
  {
    mxasio::error_code temp_ec;
    socket1.close(temp_ec);
    mxasio::detail::socket_ops::state_type state = 0;
    mxasio::detail::socket_ops::close(sv[1], state, true, temp_ec);
    MXASIO_SYNC_OP_VOID_RETURN(ec);
  }

  MXASIO_SYNC_OP_VOID_RETURN(ec);
}

} // namespace local
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // defined(MXASIO_HAS_LOCAL_SOCKETS)
       //   || defined(GENERATING_DOCUMENTATION)

#endif // MXASIO_LOCAL_CONNECT_PAIR_HPP
