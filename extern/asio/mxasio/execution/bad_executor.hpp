//
// execution/bad_executor.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_EXECUTION_BAD_EXECUTOR_HPP
#define MXASIO_EXECUTION_BAD_EXECUTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include <exception>
#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace execution {

/// Exception thrown when trying to access an empty polymorphic executor.
class bad_executor
  : public std::exception
{
public:
  /// Constructor.
  MXASIO_DECL bad_executor() noexcept;

  /// Obtain message associated with exception.
  MXASIO_DECL virtual const char* what() const noexcept;
};

} // namespace execution
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#if defined(MXASIO_HEADER_ONLY)
#include "mxasio/execution/impl/bad_executor.ipp"
#endif // defined(MXASIO_HEADER_ONLY)

#endif // MXASIO_EXECUTION_BAD_EXECUTOR_HPP
