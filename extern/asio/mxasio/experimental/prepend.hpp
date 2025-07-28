//
// experimental/prepend.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_EXPERIMENTAL_PREPEND_HPP
#define MXASIO_EXPERIMENTAL_PREPEND_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include "mxasio/prepend.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace experimental {

#if !defined(MXASIO_NO_DEPRECATED)
using mxasio::prepend_t;
using mxasio::prepend;
#endif // !defined(MXASIO_NO_DEPRECATED)

} // namespace experimental
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // MXASIO_EXPERIMENTAL_PREPEND_HPP
