//
// connect_pipe.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_CONNECT_PIPE_HPP
#define MXASIO_CONNECT_PIPE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_HAS_PIPE) \
  || defined(GENERATING_DOCUMENTATION)

#include "mxasio/basic_readable_pipe.hpp"
#include "mxasio/basic_writable_pipe.hpp"
#include "mxasio/error.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

#if defined(MXASIO_HAS_IOCP)
typedef HANDLE native_pipe_handle;
#else // defined(MXASIO_HAS_IOCP)
typedef int native_pipe_handle;
#endif // defined(MXASIO_HAS_IOCP)

MXASIO_DECL void create_pipe(native_pipe_handle p[2],
    mxasio::error_code& ec);

MXASIO_DECL void close_pipe(native_pipe_handle p);

} // namespace detail

/// Connect two pipe ends using an anonymous pipe.
/**
 * @param read_end The read end of the pipe.
 *
 * @param write_end The write end of the pipe.
 *
 * @throws mxasio::system_error Thrown on failure.
 */
template <typename Executor1, typename Executor2>
void connect_pipe(basic_readable_pipe<Executor1>& read_end,
    basic_writable_pipe<Executor2>& write_end);

/// Connect two pipe ends using an anonymous pipe.
/**
 * @param read_end The read end of the pipe.
 *
 * @param write_end The write end of the pipe.
 *
 * @throws mxasio::system_error Thrown on failure.
 *
 * @param ec Set to indicate what error occurred, if any.
 */
template <typename Executor1, typename Executor2>
MXASIO_SYNC_OP_VOID connect_pipe(basic_readable_pipe<Executor1>& read_end,
    basic_writable_pipe<Executor2>& write_end, mxasio::error_code& ec);

} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#include "mxasio/impl/connect_pipe.hpp"
#if defined(MXASIO_HEADER_ONLY)
#include "mxasio/impl/connect_pipe.ipp"
#endif // defined(MXASIO_HEADER_ONLY)

#endif // defined(MXASIO_HAS_PIPE)
       //   || defined(GENERATING_DOCUMENTATION)

#endif // MXASIO_CONNECT_PIPE_HPP
