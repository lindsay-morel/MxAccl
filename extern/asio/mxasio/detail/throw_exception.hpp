//
// detail/throw_exception.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_THROW_EXCEPTION_HPP
#define MXASIO_DETAIL_THROW_EXCEPTION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_HAS_BOOST_THROW_EXCEPTION)
# include <boost/throw_exception.hpp>
#endif // defined(MXASIO_BOOST_THROW_EXCEPTION)

namespace mxasio {
namespace detail {

#if defined(MXASIO_HAS_BOOST_THROW_EXCEPTION)
using boost::throw_exception;
#else // defined(MXASIO_HAS_BOOST_THROW_EXCEPTION)

// Declare the throw_exception function for all targets.
template <typename Exception>
void throw_exception(
    const Exception& e
    MXASIO_SOURCE_LOCATION_DEFAULTED_PARAM);

// Only define the throw_exception function when exceptions are enabled.
// Otherwise, it is up to the application to provide a definition of this
// function.
# if !defined(MXASIO_NO_EXCEPTIONS)
template <typename Exception>
void throw_exception(
    const Exception& e
    MXASIO_SOURCE_LOCATION_PARAM)
{
  throw e;
}
# endif // !defined(MXASIO_NO_EXCEPTIONS)

#endif // defined(MXASIO_HAS_BOOST_THROW_EXCEPTION)

} // namespace detail
} // namespace mxasio

#endif // MXASIO_DETAIL_THROW_EXCEPTION_HPP
