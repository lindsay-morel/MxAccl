//
// local/seq_packet_protocol.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_LOCAL_SEQ_PACKET_PROTOCOL_HPP
#define MXASIO_LOCAL_SEQ_PACKET_PROTOCOL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_HAS_LOCAL_SOCKETS) \
  || defined(GENERATING_DOCUMENTATION)

#include "mxasio/basic_socket_acceptor.hpp"
#include "mxasio/basic_seq_packet_socket.hpp"
#include "mxasio/detail/socket_types.hpp"
#include "mxasio/local/basic_endpoint.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace local {

/// Encapsulates the flags needed for seq_packet UNIX sockets.
/**
 * The mxasio::local::seq_packet_protocol class contains flags necessary
 * for sequenced packet UNIX domain sockets.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Safe.
 *
 * @par Concepts:
 * Protocol.
 */
class seq_packet_protocol
{
public:
  /// Obtain an identifier for the type of the protocol.
  int type() const noexcept
  {
    return SOCK_SEQPACKET;
  }

  /// Obtain an identifier for the protocol.
  int protocol() const noexcept
  {
    return 0;
  }

  /// Obtain an identifier for the protocol family.
  int family() const noexcept
  {
    return AF_UNIX;
  }

  /// The type of a UNIX domain endpoint.
  typedef basic_endpoint<seq_packet_protocol> endpoint;

  /// The UNIX domain socket type.
  typedef basic_seq_packet_socket<seq_packet_protocol> socket;

  /// The UNIX domain acceptor type.
  typedef basic_socket_acceptor<seq_packet_protocol> acceptor;
};

} // namespace local
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // defined(MXASIO_HAS_LOCAL_SOCKETS)
       //   || defined(GENERATING_DOCUMENTATION)

#endif // MXASIO_LOCAL_SEQ_PACKET_PROTOCOL_HPP
