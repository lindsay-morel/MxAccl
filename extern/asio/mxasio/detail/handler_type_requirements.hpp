//
// detail/handler_type_requirements.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_HANDLER_TYPE_REQUIREMENTS_HPP
#define MXASIO_DETAIL_HANDLER_TYPE_REQUIREMENTS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

// Older versions of gcc have difficulty compiling the sizeof expressions where
// we test the handler type requirements. We'll disable checking of handler type
// requirements for those compilers, but otherwise enable it by default.
#if !defined(MXASIO_DISABLE_HANDLER_TYPE_REQUIREMENTS)
# if !defined(__GNUC__) || (__GNUC__ >= 4)
#  define MXASIO_ENABLE_HANDLER_TYPE_REQUIREMENTS 1
# endif // !defined(__GNUC__) || (__GNUC__ >= 4)
#endif // !defined(MXASIO_DISABLE_HANDLER_TYPE_REQUIREMENTS)

// With C++0x we can use a combination of enhanced SFINAE and static_assert to
// generate better template error messages. As this technique is not yet widely
// portable, we'll only enable it for tested compilers.
#if !defined(MXASIO_DISABLE_HANDLER_TYPE_REQUIREMENTS_ASSERT)
# if defined(__GNUC__)
#  if ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5)) || (__GNUC__ > 4)
#   if defined(__GXX_EXPERIMENTAL_CXX0X__)
#    define MXASIO_ENABLE_HANDLER_TYPE_REQUIREMENTS_ASSERT 1
#   endif // defined(__GXX_EXPERIMENTAL_CXX0X__)
#  endif // ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5)) || (__GNUC__ > 4)
# endif // defined(__GNUC__)
# if defined(MXASIO_MSVC)
#  if (_MSC_VER >= 1600)
#   define MXASIO_ENABLE_HANDLER_TYPE_REQUIREMENTS_ASSERT 1
#  endif // (_MSC_VER >= 1600)
# endif // defined(MXASIO_MSVC)
# if defined(__clang__)
#  if __has_feature(__cxx_static_assert__)
#   define MXASIO_ENABLE_HANDLER_TYPE_REQUIREMENTS_ASSERT 1
#  endif // __has_feature(cxx_static_assert)
# endif // defined(__clang__)
#endif // !defined(MXASIO_DISABLE_HANDLER_TYPE_REQUIREMENTS)

#if defined(MXASIO_ENABLE_HANDLER_TYPE_REQUIREMENTS)
#include "mxasio/async_result.hpp"
#endif // defined(MXASIO_ENABLE_HANDLER_TYPE_REQUIREMENTS)

namespace mxasio {
namespace detail {

#if defined(MXASIO_ENABLE_HANDLER_TYPE_REQUIREMENTS)

# if defined(MXASIO_ENABLE_HANDLER_TYPE_REQUIREMENTS_ASSERT)

template <typename Handler>
auto zero_arg_copyable_handler_test(Handler h, void*)
  -> decltype(
    sizeof(Handler(static_cast<const Handler&>(h))),
    (static_cast<Handler&&>(h)()),
    char(0));

template <typename Handler>
char (&zero_arg_copyable_handler_test(Handler, ...))[2];

template <typename Handler, typename Arg1>
auto one_arg_handler_test(Handler h, Arg1* a1)
  -> decltype(
    sizeof(Handler(static_cast<Handler&&>(h))),
    (static_cast<Handler&&>(h)(*a1)),
    char(0));

template <typename Handler>
char (&one_arg_handler_test(Handler h, ...))[2];

template <typename Handler, typename Arg1, typename Arg2>
auto two_arg_handler_test(Handler h, Arg1* a1, Arg2* a2)
  -> decltype(
    sizeof(Handler(static_cast<Handler&&>(h))),
    (static_cast<Handler&&>(h)(*a1, *a2)),
    char(0));

template <typename Handler>
char (&two_arg_handler_test(Handler, ...))[2];

template <typename Handler, typename Arg1, typename Arg2>
auto two_arg_move_handler_test(Handler h, Arg1* a1, Arg2* a2)
  -> decltype(
    sizeof(Handler(static_cast<Handler&&>(h))),
    (static_cast<Handler&&>(h)(
      *a1, static_cast<Arg2&&>(*a2))),
    char(0));

template <typename Handler>
char (&two_arg_move_handler_test(Handler, ...))[2];

#  define MXASIO_HANDLER_TYPE_REQUIREMENTS_ASSERT(expr, msg) \
     static_assert(expr, msg);

# else // defined(MXASIO_ENABLE_HANDLER_TYPE_REQUIREMENTS_ASSERT)

#  define MXASIO_HANDLER_TYPE_REQUIREMENTS_ASSERT(expr, msg)

# endif // defined(MXASIO_ENABLE_HANDLER_TYPE_REQUIREMENTS_ASSERT)

template <typename T> T& lvref();
template <typename T> T& lvref(T);
template <typename T> const T& clvref();
template <typename T> const T& clvref(T);
template <typename T> T rvref();
template <typename T> T rvref(T);
template <typename T> T rorlvref();
template <typename T> char argbyv(T);

template <int>
struct handler_type_requirements
{
};

#define MXASIO_LEGACY_COMPLETION_HANDLER_CHECK( \
    handler_type, handler) \
  \
  typedef MXASIO_HANDLER_TYPE(handler_type, \
      void()) asio_true_handler_type; \
  \
  MXASIO_HANDLER_TYPE_REQUIREMENTS_ASSERT( \
      sizeof(mxasio::detail::zero_arg_copyable_handler_test( \
          mxasio::detail::clvref< \
            asio_true_handler_type>(), 0)) == 1, \
      "CompletionHandler type requirements not met") \
  \
  typedef mxasio::detail::handler_type_requirements< \
      sizeof( \
        mxasio::detail::argbyv( \
          mxasio::detail::clvref< \
            asio_true_handler_type>())) + \
      sizeof( \
        mxasio::detail::rorlvref< \
          asio_true_handler_type>()(), \
        char(0))> MXASIO_UNUSED_TYPEDEF

#define MXASIO_READ_HANDLER_CHECK( \
    handler_type, handler) \
  \
  typedef MXASIO_HANDLER_TYPE(handler_type, \
      void(mxasio::error_code, std::size_t)) \
    asio_true_handler_type; \
  \
  MXASIO_HANDLER_TYPE_REQUIREMENTS_ASSERT( \
      sizeof(mxasio::detail::two_arg_handler_test( \
          mxasio::detail::rvref< \
            asio_true_handler_type>(), \
          static_cast<const mxasio::error_code*>(0), \
          static_cast<const std::size_t*>(0))) == 1, \
      "ReadHandler type requirements not met") \
  \
  typedef mxasio::detail::handler_type_requirements< \
      sizeof( \
        mxasio::detail::argbyv( \
          mxasio::detail::rvref< \
            asio_true_handler_type>())) + \
      sizeof( \
        mxasio::detail::rorlvref< \
          asio_true_handler_type>()( \
            mxasio::detail::lvref<const mxasio::error_code>(), \
            mxasio::detail::lvref<const std::size_t>()), \
        char(0))> MXASIO_UNUSED_TYPEDEF

#define MXASIO_WRITE_HANDLER_CHECK( \
    handler_type, handler) \
  \
  typedef MXASIO_HANDLER_TYPE(handler_type, \
      void(mxasio::error_code, std::size_t)) \
    asio_true_handler_type; \
  \
  MXASIO_HANDLER_TYPE_REQUIREMENTS_ASSERT( \
      sizeof(mxasio::detail::two_arg_handler_test( \
          mxasio::detail::rvref< \
            asio_true_handler_type>(), \
          static_cast<const mxasio::error_code*>(0), \
          static_cast<const std::size_t*>(0))) == 1, \
      "WriteHandler type requirements not met") \
  \
  typedef mxasio::detail::handler_type_requirements< \
      sizeof( \
        mxasio::detail::argbyv( \
          mxasio::detail::rvref< \
            asio_true_handler_type>())) + \
      sizeof( \
        mxasio::detail::rorlvref< \
          asio_true_handler_type>()( \
            mxasio::detail::lvref<const mxasio::error_code>(), \
            mxasio::detail::lvref<const std::size_t>()), \
        char(0))> MXASIO_UNUSED_TYPEDEF

#define MXASIO_ACCEPT_HANDLER_CHECK( \
    handler_type, handler) \
  \
  typedef MXASIO_HANDLER_TYPE(handler_type, \
      void(mxasio::error_code)) \
    asio_true_handler_type; \
  \
  MXASIO_HANDLER_TYPE_REQUIREMENTS_ASSERT( \
      sizeof(mxasio::detail::one_arg_handler_test( \
          mxasio::detail::rvref< \
            asio_true_handler_type>(), \
          static_cast<const mxasio::error_code*>(0))) == 1, \
      "AcceptHandler type requirements not met") \
  \
  typedef mxasio::detail::handler_type_requirements< \
      sizeof( \
        mxasio::detail::argbyv( \
          mxasio::detail::rvref< \
            asio_true_handler_type>())) + \
      sizeof( \
        mxasio::detail::rorlvref< \
          asio_true_handler_type>()( \
            mxasio::detail::lvref<const mxasio::error_code>()), \
        char(0))> MXASIO_UNUSED_TYPEDEF

#define MXASIO_MOVE_ACCEPT_HANDLER_CHECK( \
    handler_type, handler, socket_type) \
  \
  typedef MXASIO_HANDLER_TYPE(handler_type, \
      void(mxasio::error_code, socket_type)) \
    asio_true_handler_type; \
  \
  MXASIO_HANDLER_TYPE_REQUIREMENTS_ASSERT( \
      sizeof(mxasio::detail::two_arg_move_handler_test( \
          mxasio::detail::rvref< \
            asio_true_handler_type>(), \
          static_cast<const mxasio::error_code*>(0), \
          static_cast<socket_type*>(0))) == 1, \
      "MoveAcceptHandler type requirements not met") \
  \
  typedef mxasio::detail::handler_type_requirements< \
      sizeof( \
        mxasio::detail::argbyv( \
          mxasio::detail::rvref< \
            asio_true_handler_type>())) + \
      sizeof( \
        mxasio::detail::rorlvref< \
          asio_true_handler_type>()( \
            mxasio::detail::lvref<const mxasio::error_code>(), \
            mxasio::detail::rvref<socket_type>()), \
        char(0))> MXASIO_UNUSED_TYPEDEF

#define MXASIO_CONNECT_HANDLER_CHECK( \
    handler_type, handler) \
  \
  typedef MXASIO_HANDLER_TYPE(handler_type, \
      void(mxasio::error_code)) \
    asio_true_handler_type; \
  \
  MXASIO_HANDLER_TYPE_REQUIREMENTS_ASSERT( \
      sizeof(mxasio::detail::one_arg_handler_test( \
          mxasio::detail::rvref< \
            asio_true_handler_type>(), \
          static_cast<const mxasio::error_code*>(0))) == 1, \
      "ConnectHandler type requirements not met") \
  \
  typedef mxasio::detail::handler_type_requirements< \
      sizeof( \
        mxasio::detail::argbyv( \
          mxasio::detail::rvref< \
            asio_true_handler_type>())) + \
      sizeof( \
        mxasio::detail::rorlvref< \
          asio_true_handler_type>()( \
            mxasio::detail::lvref<const mxasio::error_code>()), \
        char(0))> MXASIO_UNUSED_TYPEDEF

#define MXASIO_RANGE_CONNECT_HANDLER_CHECK( \
    handler_type, handler, endpoint_type) \
  \
  typedef MXASIO_HANDLER_TYPE(handler_type, \
      void(mxasio::error_code, endpoint_type)) \
    asio_true_handler_type; \
  \
  MXASIO_HANDLER_TYPE_REQUIREMENTS_ASSERT( \
      sizeof(mxasio::detail::two_arg_handler_test( \
          mxasio::detail::rvref< \
            asio_true_handler_type>(), \
          static_cast<const mxasio::error_code*>(0), \
          static_cast<const endpoint_type*>(0))) == 1, \
      "RangeConnectHandler type requirements not met") \
  \
  typedef mxasio::detail::handler_type_requirements< \
      sizeof( \
        mxasio::detail::argbyv( \
          mxasio::detail::rvref< \
            asio_true_handler_type>())) + \
      sizeof( \
        mxasio::detail::rorlvref< \
          asio_true_handler_type>()( \
            mxasio::detail::lvref<const mxasio::error_code>(), \
            mxasio::detail::lvref<const endpoint_type>()), \
        char(0))> MXASIO_UNUSED_TYPEDEF

#define MXASIO_ITERATOR_CONNECT_HANDLER_CHECK( \
    handler_type, handler, iter_type) \
  \
  typedef MXASIO_HANDLER_TYPE(handler_type, \
      void(mxasio::error_code, iter_type)) \
    asio_true_handler_type; \
  \
  MXASIO_HANDLER_TYPE_REQUIREMENTS_ASSERT( \
      sizeof(mxasio::detail::two_arg_handler_test( \
          mxasio::detail::rvref< \
            asio_true_handler_type>(), \
          static_cast<const mxasio::error_code*>(0), \
          static_cast<const iter_type*>(0))) == 1, \
      "IteratorConnectHandler type requirements not met") \
  \
  typedef mxasio::detail::handler_type_requirements< \
      sizeof( \
        mxasio::detail::argbyv( \
          mxasio::detail::rvref< \
            asio_true_handler_type>())) + \
      sizeof( \
        mxasio::detail::rorlvref< \
          asio_true_handler_type>()( \
            mxasio::detail::lvref<const mxasio::error_code>(), \
            mxasio::detail::lvref<const iter_type>()), \
        char(0))> MXASIO_UNUSED_TYPEDEF

#define MXASIO_RESOLVE_HANDLER_CHECK( \
    handler_type, handler, range_type) \
  \
  typedef MXASIO_HANDLER_TYPE(handler_type, \
      void(mxasio::error_code, range_type)) \
    asio_true_handler_type; \
  \
  MXASIO_HANDLER_TYPE_REQUIREMENTS_ASSERT( \
      sizeof(mxasio::detail::two_arg_handler_test( \
          mxasio::detail::rvref< \
            asio_true_handler_type>(), \
          static_cast<const mxasio::error_code*>(0), \
          static_cast<const range_type*>(0))) == 1, \
      "ResolveHandler type requirements not met") \
  \
  typedef mxasio::detail::handler_type_requirements< \
      sizeof( \
        mxasio::detail::argbyv( \
          mxasio::detail::rvref< \
            asio_true_handler_type>())) + \
      sizeof( \
        mxasio::detail::rorlvref< \
          asio_true_handler_type>()( \
            mxasio::detail::lvref<const mxasio::error_code>(), \
            mxasio::detail::lvref<const range_type>()), \
        char(0))> MXASIO_UNUSED_TYPEDEF

#define MXASIO_WAIT_HANDLER_CHECK( \
    handler_type, handler) \
  \
  typedef MXASIO_HANDLER_TYPE(handler_type, \
      void(mxasio::error_code)) \
    asio_true_handler_type; \
  \
  MXASIO_HANDLER_TYPE_REQUIREMENTS_ASSERT( \
      sizeof(mxasio::detail::one_arg_handler_test( \
          mxasio::detail::rvref< \
            asio_true_handler_type>(), \
          static_cast<const mxasio::error_code*>(0))) == 1, \
      "WaitHandler type requirements not met") \
  \
  typedef mxasio::detail::handler_type_requirements< \
      sizeof( \
        mxasio::detail::argbyv( \
          mxasio::detail::rvref< \
            asio_true_handler_type>())) + \
      sizeof( \
        mxasio::detail::rorlvref< \
          asio_true_handler_type>()( \
            mxasio::detail::lvref<const mxasio::error_code>()), \
        char(0))> MXASIO_UNUSED_TYPEDEF

#define MXASIO_SIGNAL_HANDLER_CHECK( \
    handler_type, handler) \
  \
  typedef MXASIO_HANDLER_TYPE(handler_type, \
      void(mxasio::error_code, int)) \
    asio_true_handler_type; \
  \
  MXASIO_HANDLER_TYPE_REQUIREMENTS_ASSERT( \
      sizeof(mxasio::detail::two_arg_handler_test( \
          mxasio::detail::rvref< \
            asio_true_handler_type>(), \
          static_cast<const mxasio::error_code*>(0), \
          static_cast<const int*>(0))) == 1, \
      "SignalHandler type requirements not met") \
  \
  typedef mxasio::detail::handler_type_requirements< \
      sizeof( \
        mxasio::detail::argbyv( \
          mxasio::detail::rvref< \
            asio_true_handler_type>())) + \
      sizeof( \
        mxasio::detail::rorlvref< \
          asio_true_handler_type>()( \
            mxasio::detail::lvref<const mxasio::error_code>(), \
            mxasio::detail::lvref<const int>()), \
        char(0))> MXASIO_UNUSED_TYPEDEF

#define MXASIO_HANDSHAKE_HANDLER_CHECK( \
    handler_type, handler) \
  \
  typedef MXASIO_HANDLER_TYPE(handler_type, \
      void(mxasio::error_code)) \
    asio_true_handler_type; \
  \
  MXASIO_HANDLER_TYPE_REQUIREMENTS_ASSERT( \
      sizeof(mxasio::detail::one_arg_handler_test( \
          mxasio::detail::rvref< \
            asio_true_handler_type>(), \
          static_cast<const mxasio::error_code*>(0))) == 1, \
      "HandshakeHandler type requirements not met") \
  \
  typedef mxasio::detail::handler_type_requirements< \
      sizeof( \
        mxasio::detail::argbyv( \
          mxasio::detail::rvref< \
            asio_true_handler_type>())) + \
      sizeof( \
        mxasio::detail::rorlvref< \
          asio_true_handler_type>()( \
            mxasio::detail::lvref<const mxasio::error_code>()), \
        char(0))> MXASIO_UNUSED_TYPEDEF

#define MXASIO_BUFFERED_HANDSHAKE_HANDLER_CHECK( \
    handler_type, handler) \
  \
  typedef MXASIO_HANDLER_TYPE(handler_type, \
      void(mxasio::error_code, std::size_t)) \
    asio_true_handler_type; \
  \
  MXASIO_HANDLER_TYPE_REQUIREMENTS_ASSERT( \
      sizeof(mxasio::detail::two_arg_handler_test( \
          mxasio::detail::rvref< \
            asio_true_handler_type>(), \
          static_cast<const mxasio::error_code*>(0), \
          static_cast<const std::size_t*>(0))) == 1, \
      "BufferedHandshakeHandler type requirements not met") \
  \
  typedef mxasio::detail::handler_type_requirements< \
      sizeof( \
        mxasio::detail::argbyv( \
          mxasio::detail::rvref< \
            asio_true_handler_type>())) + \
      sizeof( \
        mxasio::detail::rorlvref< \
          asio_true_handler_type>()( \
          mxasio::detail::lvref<const mxasio::error_code>(), \
          mxasio::detail::lvref<const std::size_t>()), \
        char(0))> MXASIO_UNUSED_TYPEDEF

#define MXASIO_SHUTDOWN_HANDLER_CHECK( \
    handler_type, handler) \
  \
  typedef MXASIO_HANDLER_TYPE(handler_type, \
      void(mxasio::error_code)) \
    asio_true_handler_type; \
  \
  MXASIO_HANDLER_TYPE_REQUIREMENTS_ASSERT( \
      sizeof(mxasio::detail::one_arg_handler_test( \
          mxasio::detail::rvref< \
            asio_true_handler_type>(), \
          static_cast<const mxasio::error_code*>(0))) == 1, \
      "ShutdownHandler type requirements not met") \
  \
  typedef mxasio::detail::handler_type_requirements< \
      sizeof( \
        mxasio::detail::argbyv( \
          mxasio::detail::rvref< \
            asio_true_handler_type>())) + \
      sizeof( \
        mxasio::detail::rorlvref< \
          asio_true_handler_type>()( \
            mxasio::detail::lvref<const mxasio::error_code>()), \
        char(0))> MXASIO_UNUSED_TYPEDEF

#else // !defined(MXASIO_ENABLE_HANDLER_TYPE_REQUIREMENTS)

#define MXASIO_LEGACY_COMPLETION_HANDLER_CHECK( \
    handler_type, handler) \
  typedef int MXASIO_UNUSED_TYPEDEF

#define MXASIO_READ_HANDLER_CHECK( \
    handler_type, handler) \
  typedef int MXASIO_UNUSED_TYPEDEF

#define MXASIO_WRITE_HANDLER_CHECK( \
    handler_type, handler) \
  typedef int MXASIO_UNUSED_TYPEDEF

#define MXASIO_ACCEPT_HANDLER_CHECK( \
    handler_type, handler) \
  typedef int MXASIO_UNUSED_TYPEDEF

#define MXASIO_MOVE_ACCEPT_HANDLER_CHECK( \
    handler_type, handler, socket_type) \
  typedef int MXASIO_UNUSED_TYPEDEF

#define MXASIO_CONNECT_HANDLER_CHECK( \
    handler_type, handler) \
  typedef int MXASIO_UNUSED_TYPEDEF

#define MXASIO_RANGE_CONNECT_HANDLER_CHECK( \
    handler_type, handler, iter_type) \
  typedef int MXASIO_UNUSED_TYPEDEF

#define MXASIO_ITERATOR_CONNECT_HANDLER_CHECK( \
    handler_type, handler, iter_type) \
  typedef int MXASIO_UNUSED_TYPEDEF

#define MXASIO_RESOLVE_HANDLER_CHECK( \
    handler_type, handler, iter_type) \
  typedef int MXASIO_UNUSED_TYPEDEF

#define MXASIO_WAIT_HANDLER_CHECK( \
    handler_type, handler) \
  typedef int MXASIO_UNUSED_TYPEDEF

#define MXASIO_SIGNAL_HANDLER_CHECK( \
    handler_type, handler) \
  typedef int MXASIO_UNUSED_TYPEDEF

#define MXASIO_HANDSHAKE_HANDLER_CHECK( \
    handler_type, handler) \
  typedef int MXASIO_UNUSED_TYPEDEF

#define MXASIO_BUFFERED_HANDSHAKE_HANDLER_CHECK( \
    handler_type, handler) \
  typedef int MXASIO_UNUSED_TYPEDEF

#define MXASIO_SHUTDOWN_HANDLER_CHECK( \
    handler_type, handler) \
  typedef int MXASIO_UNUSED_TYPEDEF

#endif // !defined(MXASIO_ENABLE_HANDLER_TYPE_REQUIREMENTS)

} // namespace detail
} // namespace mxasio

#endif // MXASIO_DETAIL_HANDLER_TYPE_REQUIREMENTS_HPP
