//
// ssl/detail/write_op.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_SSL_DETAIL_WRITE_OP_HPP
#define MXASIO_SSL_DETAIL_WRITE_OP_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#include "mxasio/detail/buffer_sequence_adapter.hpp"
#include "mxasio/ssl/detail/engine.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace ssl {
namespace detail {

template <typename ConstBufferSequence>
class write_op
{
public:
  static constexpr const char* tracking_name()
  {
    return "ssl::stream<>::async_write_some";
  }

  write_op(const ConstBufferSequence& buffers)
    : buffers_(buffers)
  {
  }

  engine::want operator()(engine& eng,
      mxasio::error_code& ec,
      std::size_t& bytes_transferred) const
  {
    unsigned char storage[
      mxasio::detail::buffer_sequence_adapter<mxasio::const_buffer,
        ConstBufferSequence>::linearisation_storage_size];

    mxasio::const_buffer buffer =
      mxasio::detail::buffer_sequence_adapter<mxasio::const_buffer,
        ConstBufferSequence>::linearise(buffers_, mxasio::buffer(storage));

    return eng.write(buffer, ec, bytes_transferred);
  }

  template <typename Handler>
  void call_handler(Handler& handler,
      const mxasio::error_code& ec,
      const std::size_t& bytes_transferred) const
  {
    static_cast<Handler&&>(handler)(ec, bytes_transferred);
  }

private:
  ConstBufferSequence buffers_;
};

} // namespace detail
} // namespace ssl
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // MXASIO_SSL_DETAIL_WRITE_OP_HPP
