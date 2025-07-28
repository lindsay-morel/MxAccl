//
// execution/invocable_archetype.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_EXECUTION_INVOCABLE_ARCHETYPE_HPP
#define MXASIO_EXECUTION_INVOCABLE_ARCHETYPE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include "mxasio/detail/type_traits.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace execution {

/// An archetypal function object used for determining adherence to the
/// execution::executor concept.
struct invocable_archetype
{
  /// Function call operator.
  template <typename... Args>
  void operator()(Args&&...)
  {
  }
};

} // namespace execution
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // MXASIO_EXECUTION_INVOCABLE_ARCHETYPE_HPP

