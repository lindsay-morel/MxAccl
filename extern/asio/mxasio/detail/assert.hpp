//
// detail/assert.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_ASSERT_HPP
#define MXASIO_DETAIL_ASSERT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_HAS_BOOST_ASSERT)
# include <boost/assert.hpp>
#else // defined(MXASIO_HAS_BOOST_ASSERT)
# include <cassert>
#endif // defined(MXASIO_HAS_BOOST_ASSERT)

#if defined(MXASIO_HAS_BOOST_ASSERT)
# define MXASIO_ASSERT(expr) BOOST_ASSERT(expr)
#else // defined(MXASIO_HAS_BOOST_ASSERT)
# define MXASIO_ASSERT(expr) assert(expr)
#endif // defined(MXASIO_HAS_BOOST_ASSERT)

#endif // MXASIO_DETAIL_ASSERT_HPP
