//
// version.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_VERSION_HPP
#define MXASIO_VERSION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

// MXASIO_VERSION % 100 is the sub-minor version
// MXASIO_VERSION / 100 % 1000 is the minor version
// MXASIO_VERSION / 100000 is the major version
#define MXASIO_VERSION 103002 // 1.30.2

#endif // MXASIO_VERSION_HPP
