//
// impl/connect_pipe.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_IMPL_CONNECT_PIPE_HPP
#define MXASIO_IMPL_CONNECT_PIPE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_HAS_PIPE)

#include "mxasio/connect_pipe.hpp"
#include "mxasio/detail/throw_error.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {

template <typename Executor1, typename Executor2>
void connect_pipe(basic_readable_pipe<Executor1>& read_end,
    basic_writable_pipe<Executor2>& write_end)
{
  mxasio::error_code ec;
  mxasio::connect_pipe(read_end, write_end, ec);
  mxasio::detail::throw_error(ec, "connect_pipe");
}

template <typename Executor1, typename Executor2>
MXASIO_SYNC_OP_VOID connect_pipe(basic_readable_pipe<Executor1>& read_end,
    basic_writable_pipe<Executor2>& write_end, mxasio::error_code& ec)
{
  detail::native_pipe_handle p[2];
  detail::create_pipe(p, ec);
  if (ec)
    MXASIO_SYNC_OP_VOID_RETURN(ec);

  read_end.assign(p[0], ec);
  if (ec)
  {
    detail::close_pipe(p[0]);
    detail::close_pipe(p[1]);
    MXASIO_SYNC_OP_VOID_RETURN(ec);
  }

  write_end.assign(p[1], ec);
  if (ec)
  {
    mxasio::error_code temp_ec;
    read_end.close(temp_ec);
    detail::close_pipe(p[1]);
    MXASIO_SYNC_OP_VOID_RETURN(ec);
  }

  MXASIO_SYNC_OP_VOID_RETURN(ec);
}

} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // defined(MXASIO_HAS_PIPE)

#endif // MXASIO_IMPL_CONNECT_PIPE_HPP
