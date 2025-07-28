//
// detail/local_free_on_block_exit.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_LOCAL_FREE_ON_BLOCK_EXIT_HPP
#define MXASIO_DETAIL_LOCAL_FREE_ON_BLOCK_EXIT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_WINDOWS) || defined(__CYGWIN__)
#if !defined(MXASIO_WINDOWS_APP)

#include "mxasio/detail/noncopyable.hpp"
#include "mxasio/detail/socket_types.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

class local_free_on_block_exit
  : private noncopyable
{
public:
  // Constructor blocks all signals for the calling thread.
  explicit local_free_on_block_exit(void* p)
    : p_(p)
  {
  }

  // Destructor restores the previous signal mask.
  ~local_free_on_block_exit()
  {
    ::LocalFree(p_);
  }

private:
  void* p_;
};

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // !defined(MXASIO_WINDOWS_APP)
#endif // defined(MXASIO_WINDOWS) || defined(__CYGWIN__)

#endif // MXASIO_DETAIL_LOCAL_FREE_ON_BLOCK_EXIT_HPP
