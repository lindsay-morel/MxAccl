//
// multiple_exceptions.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_MULTIPLE_EXCEPTIONS_HPP
#define MXASIO_MULTIPLE_EXCEPTIONS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include <exception>
#include "mxasio/detail/push_options.hpp"

namespace mxasio {

/// Exception thrown when there are multiple pending exceptions to rethrow.
class multiple_exceptions
  : public std::exception
{
public:
  /// Constructor.
  MXASIO_DECL multiple_exceptions(
      std::exception_ptr first) noexcept;

  /// Obtain message associated with exception.
  MXASIO_DECL virtual const char* what() const
    noexcept;

  /// Obtain a pointer to the first exception.
  MXASIO_DECL std::exception_ptr first_exception() const;

private:
  std::exception_ptr first_;
};

} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#if defined(MXASIO_HEADER_ONLY)
#include "mxasio/impl/multiple_exceptions.ipp"
#endif // defined(MXASIO_HEADER_ONLY)

#endif // MXASIO_MULTIPLE_EXCEPTIONS_HPP
