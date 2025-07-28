//
// io_service.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_IO_SERVICE_HPP
#define MXASIO_IO_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/io_context.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {

#if !defined(MXASIO_NO_DEPRECATED)
/// Typedef for backwards compatibility.
typedef io_context io_service;
#endif // !defined(MXASIO_NO_DEPRECATED)

} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // MXASIO_IO_SERVICE_HPP
