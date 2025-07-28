//
// detail/source_location.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_SOURCE_LOCATION_HPP
#define MXASIO_DETAIL_SOURCE_LOCATION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_HAS_SOURCE_LOCATION)

#if defined(MXASIO_HAS_STD_SOURCE_LOCATION)
# include <source_location>
#elif defined(MXASIO_HAS_STD_EXPERIMENTAL_SOURCE_LOCATION)
# include <experimental/source_location>
#else // defined(MXASIO_HAS_STD_EXPERIMENTAL_SOURCE_LOCATION)
# error MXASIO_HAS_SOURCE_LOCATION is set \
  but no source_location is available
#endif // defined(MXASIO_HAS_STD_EXPERIMENTAL_SOURCE_LOCATION)

namespace mxasio {
namespace detail {

#if defined(MXASIO_HAS_STD_SOURCE_LOCATION)
using std::source_location;
#elif defined(MXASIO_HAS_STD_EXPERIMENTAL_SOURCE_LOCATION)
using std::experimental::source_location;
#endif // defined(MXASIO_HAS_STD_EXPERIMENTAL_SOURCE_LOCATION)

} // namespace detail
} // namespace mxasio

#endif // defined(MXASIO_HAS_SOURCE_LOCATION)

#endif // MXASIO_DETAIL_SOURCE_LOCATION_HPP
