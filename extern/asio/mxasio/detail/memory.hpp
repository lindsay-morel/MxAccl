//
// detail/memory.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_MEMORY_HPP
#define MXASIO_DETAIL_MEMORY_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <new>
#include "mxasio/detail/cstdint.hpp"
#include "mxasio/detail/throw_exception.hpp"

#if !defined(MXASIO_HAS_STD_ALIGNED_ALLOC) \
  && defined(MXASIO_HAS_BOOST_ALIGN)
# include <boost/align/aligned_alloc.hpp>
#endif // !defined(MXASIO_HAS_STD_ALIGNED_ALLOC)
       //   && defined(MXASIO_HAS_BOOST_ALIGN)

namespace mxasio {
namespace detail {

using std::allocate_shared;
using std::make_shared;
using std::shared_ptr;
using std::weak_ptr;
using std::addressof;

#if defined(MXASIO_HAS_STD_TO_ADDRESS)
using std::to_address;
#else // defined(MXASIO_HAS_STD_TO_ADDRESS)
template <typename T>
inline T* to_address(T* p) { return p; }
template <typename T>
inline const T* to_address(const T* p) { return p; }
template <typename T>
inline volatile T* to_address(volatile T* p) { return p; }
template <typename T>
inline const volatile T* to_address(const volatile T* p) { return p; }
#endif // defined(MXASIO_HAS_STD_TO_ADDRESS)

inline void* align(std::size_t alignment,
    std::size_t size, void*& ptr, std::size_t& space)
{
  return std::align(alignment, size, ptr, space);
}

} // namespace detail

using std::allocator_arg_t;
# define MXASIO_USES_ALLOCATOR(t) \
  namespace std { \
    template <typename Allocator> \
    struct uses_allocator<t, Allocator> : true_type {}; \
  } \
  /**/
# define MXASIO_REBIND_ALLOC(alloc, t) \
  typename std::allocator_traits<alloc>::template rebind_alloc<t>
  /**/

inline void* aligned_new(std::size_t align, std::size_t size)
{
#if defined(MXASIO_HAS_STD_ALIGNED_ALLOC)
  align = (align < MXASIO_DEFAULT_ALIGN) ? MXASIO_DEFAULT_ALIGN : align;
  size = (size % align == 0) ? size : size + (align - size % align);
  void* ptr = std::aligned_alloc(align, size);
  if (!ptr)
  {
    std::bad_alloc ex;
    mxasio::detail::throw_exception(ex);
  }
  return ptr;
#elif defined(MXASIO_HAS_BOOST_ALIGN)
  align = (align < MXASIO_DEFAULT_ALIGN) ? MXASIO_DEFAULT_ALIGN : align;
  size = (size % align == 0) ? size : size + (align - size % align);
  void* ptr = boost::alignment::aligned_alloc(align, size);
  if (!ptr)
  {
    std::bad_alloc ex;
    mxasio::detail::throw_exception(ex);
  }
  return ptr;
#elif defined(MXASIO_MSVC)
  align = (align < MXASIO_DEFAULT_ALIGN) ? MXASIO_DEFAULT_ALIGN : align;
  size = (size % align == 0) ? size : size + (align - size % align);
  void* ptr = _aligned_malloc(size, align);
  if (!ptr)
  {
    std::bad_alloc ex;
    mxasio::detail::throw_exception(ex);
  }
  return ptr;
#else // defined(MXASIO_MSVC)
  (void)align;
  return ::operator new(size);
#endif // defined(MXASIO_MSVC)
}

inline void aligned_delete(void* ptr)
{
#if defined(MXASIO_HAS_STD_ALIGNED_ALLOC)
  std::free(ptr);
#elif defined(MXASIO_HAS_BOOST_ALIGN)
  boost::alignment::aligned_free(ptr);
#elif defined(MXASIO_MSVC)
  _aligned_free(ptr);
#else // defined(MXASIO_MSVC)
  ::operator delete(ptr);
#endif // defined(MXASIO_MSVC)
}

} // namespace mxasio

#endif // MXASIO_DETAIL_MEMORY_HPP
