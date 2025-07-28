//
// serial_port_base.hpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2008 Rep Invariant Systems, Inc. (info@repinvariant.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_SERIAL_PORT_BASE_HPP
#define MXASIO_SERIAL_PORT_BASE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_HAS_SERIAL_PORT) \
  || defined(GENERATING_DOCUMENTATION)

#if !defined(MXASIO_WINDOWS) && !defined(__CYGWIN__)
# include <termios.h>
#endif // !defined(MXASIO_WINDOWS) && !defined(__CYGWIN__)

#include "mxasio/detail/socket_types.hpp"
#include "mxasio/error_code.hpp"

#if defined(GENERATING_DOCUMENTATION)
# define MXASIO_OPTION_STORAGE implementation_defined
#elif defined(MXASIO_WINDOWS) || defined(__CYGWIN__)
# define MXASIO_OPTION_STORAGE DCB
#else
# define MXASIO_OPTION_STORAGE termios
#endif

#include "mxasio/detail/push_options.hpp"

namespace mxasio {

/// The serial_port_base class is used as a base for the basic_serial_port class
/// template so that we have a common place to define the serial port options.
class serial_port_base
{
public:
  /// Serial port option to permit changing the baud rate.
  /**
   * Implements changing the baud rate for a given serial port.
   */
  class baud_rate
  {
  public:
    explicit baud_rate(unsigned int rate = 0);
    unsigned int value() const;
    MXASIO_DECL MXASIO_SYNC_OP_VOID store(
        MXASIO_OPTION_STORAGE& storage,
        mxasio::error_code& ec) const;
    MXASIO_DECL MXASIO_SYNC_OP_VOID load(
        const MXASIO_OPTION_STORAGE& storage,
        mxasio::error_code& ec);
  private:
    unsigned int value_;
  };

  /// Serial port option to permit changing the flow control.
  /**
   * Implements changing the flow control for a given serial port.
   */
  class flow_control
  {
  public:
    enum type { none, software, hardware };
    MXASIO_DECL explicit flow_control(type t = none);
    type value() const;
    MXASIO_DECL MXASIO_SYNC_OP_VOID store(
        MXASIO_OPTION_STORAGE& storage,
        mxasio::error_code& ec) const;
    MXASIO_DECL MXASIO_SYNC_OP_VOID load(
        const MXASIO_OPTION_STORAGE& storage,
        mxasio::error_code& ec);
  private:
    type value_;
  };

  /// Serial port option to permit changing the parity.
  /**
   * Implements changing the parity for a given serial port.
   */
  class parity
  {
  public:
    enum type { none, odd, even };
    MXASIO_DECL explicit parity(type t = none);
    type value() const;
    MXASIO_DECL MXASIO_SYNC_OP_VOID store(
        MXASIO_OPTION_STORAGE& storage,
        mxasio::error_code& ec) const;
    MXASIO_DECL MXASIO_SYNC_OP_VOID load(
        const MXASIO_OPTION_STORAGE& storage,
        mxasio::error_code& ec);
  private:
    type value_;
  };

  /// Serial port option to permit changing the number of stop bits.
  /**
   * Implements changing the number of stop bits for a given serial port.
   */
  class stop_bits
  {
  public:
    enum type { one, onepointfive, two };
    MXASIO_DECL explicit stop_bits(type t = one);
    type value() const;
    MXASIO_DECL MXASIO_SYNC_OP_VOID store(
        MXASIO_OPTION_STORAGE& storage,
        mxasio::error_code& ec) const;
    MXASIO_DECL MXASIO_SYNC_OP_VOID load(
        const MXASIO_OPTION_STORAGE& storage,
        mxasio::error_code& ec);
  private:
    type value_;
  };

  /// Serial port option to permit changing the character size.
  /**
   * Implements changing the character size for a given serial port.
   */
  class character_size
  {
  public:
    MXASIO_DECL explicit character_size(unsigned int t = 8);
    unsigned int value() const;
    MXASIO_DECL MXASIO_SYNC_OP_VOID store(
        MXASIO_OPTION_STORAGE& storage,
        mxasio::error_code& ec) const;
    MXASIO_DECL MXASIO_SYNC_OP_VOID load(
        const MXASIO_OPTION_STORAGE& storage,
        mxasio::error_code& ec);
  private:
    unsigned int value_;
  };

protected:
  /// Protected destructor to prevent deletion through this type.
  ~serial_port_base()
  {
  }
};

} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#undef MXASIO_OPTION_STORAGE

#include "mxasio/impl/serial_port_base.hpp"
#if defined(MXASIO_HEADER_ONLY)
#include "mxasio/impl/serial_port_base.ipp"
#endif // defined(MXASIO_HEADER_ONLY)

#endif // defined(MXASIO_HAS_SERIAL_PORT)
       //   || defined(GENERATING_DOCUMENTATION)

#endif // MXASIO_SERIAL_PORT_BASE_HPP
