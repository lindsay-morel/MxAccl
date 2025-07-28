//
// detail/impl/posix_serial_port_service.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2008 Rep Invariant Systems, Inc. (info@repinvariant.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_IMPL_POSIX_SERIAL_PORT_SERVICE_IPP
#define MXASIO_DETAIL_IMPL_POSIX_SERIAL_PORT_SERVICE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_HAS_SERIAL_PORT)
#if !defined(MXASIO_WINDOWS) && !defined(__CYGWIN__)

#include <cstring>
#include "mxasio/detail/posix_serial_port_service.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

posix_serial_port_service::posix_serial_port_service(
    execution_context& context)
  : execution_context_service_base<posix_serial_port_service>(context),
    descriptor_service_(context)
{
}

void posix_serial_port_service::shutdown()
{
  descriptor_service_.shutdown();
}

mxasio::error_code posix_serial_port_service::open(
    posix_serial_port_service::implementation_type& impl,
    const std::string& device, mxasio::error_code& ec)
{
  if (is_open(impl))
  {
    ec = mxasio::error::already_open;
    MXASIO_ERROR_LOCATION(ec);
    return ec;
  }

  descriptor_ops::state_type state = 0;
  int fd = descriptor_ops::open(device.c_str(),
      O_RDWR | O_NONBLOCK | O_NOCTTY, ec);
  if (fd < 0)
  {
    MXASIO_ERROR_LOCATION(ec);
    return ec;
  }

  int s = descriptor_ops::fcntl(fd, F_GETFL, ec);
  if (s >= 0)
    s = descriptor_ops::fcntl(fd, F_SETFL, s | O_NONBLOCK, ec);
  if (s < 0)
  {
    mxasio::error_code ignored_ec;
    descriptor_ops::close(fd, state, ignored_ec);
    MXASIO_ERROR_LOCATION(ec);
    return ec;
  }

  // Set up default serial port options.
  termios ios;
  s = ::tcgetattr(fd, &ios);
  descriptor_ops::get_last_error(ec, s < 0);
  if (s >= 0)
  {
#if defined(_BSD_SOURCE) || defined(_DEFAULT_SOURCE)
    ::cfmakeraw(&ios);
#else
    ios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK
        | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    ios.c_oflag &= ~OPOST;
    ios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    ios.c_cflag &= ~(CSIZE | PARENB);
    ios.c_cflag |= CS8;
#endif
    ios.c_iflag |= IGNPAR;
    ios.c_cflag |= CREAD | CLOCAL;
    s = ::tcsetattr(fd, TCSANOW, &ios);
    descriptor_ops::get_last_error(ec, s < 0);
  }
  if (s < 0)
  {
    mxasio::error_code ignored_ec;
    descriptor_ops::close(fd, state, ignored_ec);
    MXASIO_ERROR_LOCATION(ec);
    return ec;
  }

  // We're done. Take ownership of the serial port descriptor.
  if (descriptor_service_.assign(impl, fd, ec))
  {
    mxasio::error_code ignored_ec;
    descriptor_ops::close(fd, state, ignored_ec);
  }

  MXASIO_ERROR_LOCATION(ec);
  return ec;
}

mxasio::error_code posix_serial_port_service::do_set_option(
    posix_serial_port_service::implementation_type& impl,
    posix_serial_port_service::store_function_type store,
    const void* option, mxasio::error_code& ec)
{
  termios ios;
  int s = ::tcgetattr(descriptor_service_.native_handle(impl), &ios);
  descriptor_ops::get_last_error(ec, s < 0);
  if (s < 0)
  {
    MXASIO_ERROR_LOCATION(ec);
    return ec;
  }

  if (store(option, ios, ec))
  {
    MXASIO_ERROR_LOCATION(ec);
    return ec;
  }

  s = ::tcsetattr(descriptor_service_.native_handle(impl), TCSANOW, &ios);
  descriptor_ops::get_last_error(ec, s < 0);
  MXASIO_ERROR_LOCATION(ec);
  return ec;
}

mxasio::error_code posix_serial_port_service::do_get_option(
    const posix_serial_port_service::implementation_type& impl,
    posix_serial_port_service::load_function_type load,
    void* option, mxasio::error_code& ec) const
{
  termios ios;
  int s = ::tcgetattr(descriptor_service_.native_handle(impl), &ios);
  descriptor_ops::get_last_error(ec, s < 0);
  if (s < 0)
  {
    MXASIO_ERROR_LOCATION(ec);
    return ec;
  }

  load(option, ios, ec);
  MXASIO_ERROR_LOCATION(ec);
  return ec;
}

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // !defined(MXASIO_WINDOWS) && !defined(__CYGWIN__)
#endif // defined(MXASIO_HAS_SERIAL_PORT)

#endif // MXASIO_DETAIL_IMPL_POSIX_SERIAL_PORT_SERVICE_IPP
