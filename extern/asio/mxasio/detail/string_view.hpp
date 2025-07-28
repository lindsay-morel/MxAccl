//
// detail/string_view.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_STRING_VIEW_HPP
#define MXASIO_DETAIL_STRING_VIEW_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_HAS_STRING_VIEW)

#if defined(MXASIO_HAS_STD_STRING_VIEW)
# include <string_view>
#elif defined(MXASIO_HAS_STD_EXPERIMENTAL_STRING_VIEW)
# include <experimental/string_view>
#else // defined(MXASIO_HAS_STD_EXPERIMENTAL_STRING_VIEW)
# error MXASIO_HAS_STRING_VIEW is set but no string_view is available
#endif // defined(MXASIO_HAS_STD_EXPERIMENTAL_STRING_VIEW)

namespace mxasio {

#if defined(MXASIO_HAS_STD_STRING_VIEW)
using std::basic_string_view;
using std::string_view;
#elif defined(MXASIO_HAS_STD_EXPERIMENTAL_STRING_VIEW)
using std::experimental::basic_string_view;
using std::experimental::string_view;
#endif // defined(MXASIO_HAS_STD_EXPERIMENTAL_STRING_VIEW)

} // namespace mxasio

# define MXASIO_STRING_VIEW_PARAM mxasio::string_view
#else // defined(MXASIO_HAS_STRING_VIEW)
# define MXASIO_STRING_VIEW_PARAM const std::string&
#endif // defined(MXASIO_HAS_STRING_VIEW)

#endif // MXASIO_DETAIL_STRING_VIEW_HPP
