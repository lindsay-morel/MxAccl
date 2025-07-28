//
// detail/fenced_block.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_FENCED_BLOCK_HPP
#define MXASIO_DETAIL_FENCED_BLOCK_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if !defined(MXASIO_HAS_THREADS) \
  || defined(MXASIO_DISABLE_FENCED_BLOCK)
#include "mxasio/detail/null_fenced_block.hpp"
#else
#include "mxasio/detail/std_fenced_block.hpp"
#endif

namespace mxasio {
namespace detail {

#if !defined(MXASIO_HAS_THREADS) \
  || defined(MXASIO_DISABLE_FENCED_BLOCK)
typedef null_fenced_block fenced_block;
#else
typedef std_fenced_block fenced_block;
#endif

} // namespace detail
} // namespace mxasio

#endif // MXASIO_DETAIL_FENCED_BLOCK_HPP
