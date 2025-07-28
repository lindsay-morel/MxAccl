//
// writable_pipe.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_WRITABLE_PIPE_HPP
#define MXASIO_WRITABLE_PIPE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_HAS_PIPE) \
  || defined(GENERATING_DOCUMENTATION)

#include "mxasio/basic_writable_pipe.hpp"

namespace mxasio {

/// Typedef for the typical usage of a writable pipe.
typedef basic_writable_pipe<> writable_pipe;

} // namespace mxasio

#endif // defined(MXASIO_HAS_PIPE)
       //   || defined(GENERATING_DOCUMENTATION)

#endif // MXASIO_WRITABLE_PIPE_HPP
