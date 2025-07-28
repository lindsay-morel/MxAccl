//
// impl/ssl/src.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_SSL_IMPL_SRC_HPP
#define MXASIO_SSL_IMPL_SRC_HPP

#define MXASIO_SOURCE

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_HEADER_ONLY)
# error Do not compile Asio library source with MXASIO_HEADER_ONLY defined
#endif

#include "mxasio/ssl/impl/context.ipp"
#include "mxasio/ssl/impl/error.ipp"
#include "mxasio/ssl/detail/impl/engine.ipp"
#include "mxasio/ssl/detail/impl/openssl_init.ipp"
#include "mxasio/ssl/impl/host_name_verification.ipp"
#include "mxasio/ssl/impl/rfc2818_verification.ipp"

#endif // MXASIO_SSL_IMPL_SRC_HPP
