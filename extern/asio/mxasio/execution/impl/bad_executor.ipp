//
// exection/impl/bad_executor.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_EXECUTION_IMPL_BAD_EXECUTOR_IPP
#define MXASIO_EXECUTION_IMPL_BAD_EXECUTOR_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include "mxasio/execution/bad_executor.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace execution {

bad_executor::bad_executor() noexcept
{
}

const char* bad_executor::what() const noexcept
{
  return "bad executor";
}

} // namespace execution
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // MXASIO_EXECUTION_IMPL_BAD_EXECUTOR_IPP
