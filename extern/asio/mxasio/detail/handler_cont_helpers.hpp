//
// detail/handler_cont_helpers.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_HANDLER_CONT_HELPERS_HPP
#define MXASIO_DETAIL_HANDLER_CONT_HELPERS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include "mxasio/detail/memory.hpp"
#include "mxasio/handler_continuation_hook.hpp"

#include "mxasio/detail/push_options.hpp"

// Calls to asio_handler_is_continuation must be made from a namespace that
// does not contain overloads of this function. This namespace is defined here
// for that purpose.
namespace mxasio_handler_cont_helpers {

template <typename Context>
inline bool is_continuation(Context& context)
{
#if !defined(MXASIO_HAS_HANDLER_HOOKS)
  return false;
#else
  using mxasio::asio_handler_is_continuation;
  return asio_handler_is_continuation(
      mxasio::detail::addressof(context));
#endif
}

} // namespace mxasio_handler_cont_helpers

#include "mxasio/detail/pop_options.hpp"

#endif // MXASIO_DETAIL_HANDLER_CONT_HELPERS_HPP
