//
// detail/impl/thread_context.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_IMPL_THREAD_CONTEXT_IPP
#define MXASIO_DETAIL_IMPL_THREAD_CONTEXT_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

thread_info_base* thread_context::top_of_thread_call_stack()
{
  return thread_call_stack::top();
}

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // MXASIO_DETAIL_IMPL_THREAD_CONTEXT_IPP
