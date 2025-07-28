//
// detail/socket_select_interrupter.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_SOCKET_SELECT_INTERRUPTER_HPP
#define MXASIO_DETAIL_SOCKET_SELECT_INTERRUPTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if !defined(MXASIO_WINDOWS_RUNTIME)

#if defined(MXASIO_WINDOWS) \
  || defined(__CYGWIN__) \
  || defined(__SYMBIAN32__)

#include "mxasio/detail/socket_types.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

class socket_select_interrupter
{
public:
  // Constructor.
  MXASIO_DECL socket_select_interrupter();

  // Destructor.
  MXASIO_DECL ~socket_select_interrupter();

  // Recreate the interrupter's descriptors. Used after a fork.
  MXASIO_DECL void recreate();

  // Interrupt the select call.
  MXASIO_DECL void interrupt();

  // Reset the select interrupter. Returns true if the reset was successful.
  MXASIO_DECL bool reset();

  // Get the read descriptor to be passed to select.
  socket_type read_descriptor() const
  {
    return read_descriptor_;
  }

private:
  // Open the descriptors. Throws on error.
  MXASIO_DECL void open_descriptors();

  // Close the descriptors.
  MXASIO_DECL void close_descriptors();

  // The read end of a connection used to interrupt the select call. This file
  // descriptor is passed to select such that when it is time to stop, a single
  // byte will be written on the other end of the connection and this
  // descriptor will become readable.
  socket_type read_descriptor_;

  // The write end of a connection used to interrupt the select call. A single
  // byte may be written to this to wake up the select which is waiting for the
  // other end to become readable.
  socket_type write_descriptor_;
};

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#if defined(MXASIO_HEADER_ONLY)
#include "mxasio/detail/impl/socket_select_interrupter.ipp"
#endif // defined(MXASIO_HEADER_ONLY)

#endif // defined(MXASIO_WINDOWS)
       // || defined(__CYGWIN__)
       // || defined(__SYMBIAN32__)

#endif // !defined(MXASIO_WINDOWS_RUNTIME)

#endif // MXASIO_DETAIL_SOCKET_SELECT_INTERRUPTER_HPP
