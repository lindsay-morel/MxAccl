//
// experimental/concurrent_channel.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_EXPERIMENTAL_CONCURRENT_CHANNEL_HPP
#define MXASIO_EXPERIMENTAL_CONCURRENT_CHANNEL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include "mxasio/any_io_executor.hpp"
#include "mxasio/detail/type_traits.hpp"
#include "mxasio/execution/executor.hpp"
#include "mxasio/is_executor.hpp"
#include "mxasio/experimental/basic_concurrent_channel.hpp"
#include "mxasio/experimental/channel_traits.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace experimental {
namespace detail {

template <typename ExecutorOrSignature, typename = void>
struct concurrent_channel_type
{
  template <typename... Signatures>
  struct inner
  {
    typedef basic_concurrent_channel<any_io_executor, channel_traits<>,
        ExecutorOrSignature, Signatures...> type;
  };
};

template <typename ExecutorOrSignature>
struct concurrent_channel_type<ExecutorOrSignature,
    enable_if_t<
      is_executor<ExecutorOrSignature>::value
        || execution::is_executor<ExecutorOrSignature>::value
    >>
{
  template <typename... Signatures>
  struct inner
  {
    typedef basic_concurrent_channel<ExecutorOrSignature,
        channel_traits<>, Signatures...> type;
  };
};

} // namespace detail

/// Template type alias for common use of channel.
template <typename ExecutorOrSignature, typename... Signatures>
using concurrent_channel = typename detail::concurrent_channel_type<
    ExecutorOrSignature>::template inner<Signatures...>::type;

} // namespace experimental
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // MXASIO_EXPERIMENTAL_CONCURRENT_CHANNEL_HPP
