//
// detail/descriptor_ops.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_DESCRIPTOR_OPS_HPP
#define MXASIO_DETAIL_DESCRIPTOR_OPS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if !defined(MXASIO_WINDOWS) \
  && !defined(MXASIO_WINDOWS_RUNTIME) \
  && !defined(__CYGWIN__)

#include <cstddef>
#include "mxasio/error.hpp"
#include "mxasio/error_code.hpp"
#include "mxasio/detail/cstdint.hpp"
#include "mxasio/detail/socket_types.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {
namespace descriptor_ops {

// Descriptor state bits.
enum
{
  // The user wants a non-blocking descriptor.
  user_set_non_blocking = 1,

  // The descriptor has been set non-blocking.
  internal_non_blocking = 2,

  // Helper "state" used to determine whether the descriptor is non-blocking.
  non_blocking = user_set_non_blocking | internal_non_blocking,

  // The descriptor may have been dup()-ed.
  possible_dup = 4
};

typedef unsigned char state_type;

inline void get_last_error(
    mxasio::error_code& ec, bool is_error_condition)
{
  if (!is_error_condition)
  {
    mxasio::error::clear(ec);
  }
  else
  {
    ec = mxasio::error_code(errno,
        mxasio::error::get_system_category());
  }
}

MXASIO_DECL int open(const char* path, int flags,
    mxasio::error_code& ec);

MXASIO_DECL int open(const char* path, int flags, unsigned mode,
    mxasio::error_code& ec);

MXASIO_DECL int close(int d, state_type& state,
    mxasio::error_code& ec);

MXASIO_DECL bool set_user_non_blocking(int d,
    state_type& state, bool value, mxasio::error_code& ec);

MXASIO_DECL bool set_internal_non_blocking(int d,
    state_type& state, bool value, mxasio::error_code& ec);

typedef iovec buf;

MXASIO_DECL std::size_t sync_read(int d, state_type state, buf* bufs,
    std::size_t count, bool all_empty, mxasio::error_code& ec);

MXASIO_DECL std::size_t sync_read1(int d, state_type state, void* data,
    std::size_t size, mxasio::error_code& ec);

MXASIO_DECL bool non_blocking_read(int d, buf* bufs, std::size_t count,
    mxasio::error_code& ec, std::size_t& bytes_transferred);

MXASIO_DECL bool non_blocking_read1(int d, void* data, std::size_t size,
    mxasio::error_code& ec, std::size_t& bytes_transferred);

MXASIO_DECL std::size_t sync_write(int d, state_type state,
    const buf* bufs, std::size_t count, bool all_empty,
    mxasio::error_code& ec);

MXASIO_DECL std::size_t sync_write1(int d, state_type state,
    const void* data, std::size_t size, mxasio::error_code& ec);

MXASIO_DECL bool non_blocking_write(int d,
    const buf* bufs, std::size_t count,
    mxasio::error_code& ec, std::size_t& bytes_transferred);

MXASIO_DECL bool non_blocking_write1(int d,
    const void* data, std::size_t size,
    mxasio::error_code& ec, std::size_t& bytes_transferred);

#if defined(MXASIO_HAS_FILE)

MXASIO_DECL std::size_t sync_read_at(int d, state_type state,
    uint64_t offset, buf* bufs, std::size_t count, bool all_empty,
    mxasio::error_code& ec);

MXASIO_DECL std::size_t sync_read_at1(int d, state_type state,
    uint64_t offset, void* data, std::size_t size,
    mxasio::error_code& ec);

MXASIO_DECL bool non_blocking_read_at(int d, uint64_t offset,
    buf* bufs, std::size_t count, mxasio::error_code& ec,
    std::size_t& bytes_transferred);

MXASIO_DECL bool non_blocking_read_at1(int d, uint64_t offset,
    void* data, std::size_t size, mxasio::error_code& ec,
    std::size_t& bytes_transferred);

MXASIO_DECL std::size_t sync_write_at(int d, state_type state,
    uint64_t offset, const buf* bufs, std::size_t count, bool all_empty,
    mxasio::error_code& ec);

MXASIO_DECL std::size_t sync_write_at1(int d, state_type state,
    uint64_t offset, const void* data, std::size_t size,
    mxasio::error_code& ec);

MXASIO_DECL bool non_blocking_write_at(int d,
    uint64_t offset, const buf* bufs, std::size_t count,
    mxasio::error_code& ec, std::size_t& bytes_transferred);

MXASIO_DECL bool non_blocking_write_at1(int d,
    uint64_t offset, const void* data, std::size_t size,
    mxasio::error_code& ec, std::size_t& bytes_transferred);

#endif // defined(MXASIO_HAS_FILE)

MXASIO_DECL int ioctl(int d, state_type& state, long cmd,
    ioctl_arg_type* arg, mxasio::error_code& ec);

MXASIO_DECL int fcntl(int d, int cmd, mxasio::error_code& ec);

MXASIO_DECL int fcntl(int d, int cmd,
    long arg, mxasio::error_code& ec);

MXASIO_DECL int poll_read(int d,
    state_type state, mxasio::error_code& ec);

MXASIO_DECL int poll_write(int d,
    state_type state, mxasio::error_code& ec);

MXASIO_DECL int poll_error(int d,
    state_type state, mxasio::error_code& ec);

} // namespace descriptor_ops
} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#if defined(MXASIO_HEADER_ONLY)
#include "mxasio/detail/impl/descriptor_ops.ipp"
#endif // defined(MXASIO_HEADER_ONLY)

#endif // !defined(MXASIO_WINDOWS)
       //   && !defined(MXASIO_WINDOWS_RUNTIME)
       //   && !defined(__CYGWIN__)

#endif // MXASIO_DETAIL_DESCRIPTOR_OPS_HPP
