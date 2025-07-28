//
// experimental/channel_error.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_EXPERIMENTAL_CHANNEL_ERROR_HPP
#define MXASIO_EXPERIMENTAL_CHANNEL_ERROR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include "mxasio/error_code.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace experimental {
namespace error {

enum channel_errors
{
  /// The channel was closed.
  channel_closed = 1,

  /// The channel was cancelled.
  channel_cancelled = 2
};

extern MXASIO_DECL
const mxasio::error_category& get_channel_category();

static const mxasio::error_category&
  channel_category MXASIO_UNUSED_VARIABLE
  = mxasio::experimental::error::get_channel_category();

} // namespace error
namespace channel_errc {
  // Simulates a scoped enum.
  using error::channel_closed;
  using error::channel_cancelled;
} // namespace channel_errc
} // namespace experimental
} // namespace mxasio

namespace std {

template<> struct is_error_code_enum<
    mxasio::experimental::error::channel_errors>
{
  static const bool value = true;
};

} // namespace std

namespace mxasio {
namespace experimental {
namespace error {

inline mxasio::error_code make_error_code(channel_errors e)
{
  return mxasio::error_code(
      static_cast<int>(e), get_channel_category());
}

} // namespace error
} // namespace experimental
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#if defined(MXASIO_HEADER_ONLY)
#include "mxasio/experimental/impl/channel_error.ipp"
#endif // defined(MXASIO_HEADER_ONLY)

#endif // MXASIO_EXPERIMENTAL_CHANNEL_ERROR_HPP
