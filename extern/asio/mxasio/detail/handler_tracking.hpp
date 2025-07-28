//
// detail/handler_tracking.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_HANDLER_TRACKING_HPP
#define MXASIO_DETAIL_HANDLER_TRACKING_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

namespace mxasio {

class execution_context;

} // namespace mxasio

#if defined(MXASIO_CUSTOM_HANDLER_TRACKING)
# include MXASIO_CUSTOM_HANDLER_TRACKING
#elif defined(MXASIO_ENABLE_HANDLER_TRACKING)
#include "mxasio/error_code.hpp"
#include "mxasio/detail/cstdint.hpp"
#include "mxasio/detail/static_mutex.hpp"
#include "mxasio/detail/tss_ptr.hpp"
#endif // defined(MXASIO_ENABLE_HANDLER_TRACKING)

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

#if defined(MXASIO_CUSTOM_HANDLER_TRACKING)

// The user-specified header must define the following macros:
// - MXASIO_INHERIT_TRACKED_HANDLER
// - MXASIO_ALSO_INHERIT_TRACKED_HANDLER
// - MXASIO_HANDLER_TRACKING_INIT
// - MXASIO_HANDLER_CREATION(args)
// - MXASIO_HANDLER_COMPLETION(args)
// - MXASIO_HANDLER_INVOCATION_BEGIN(args)
// - MXASIO_HANDLER_INVOCATION_END
// - MXASIO_HANDLER_OPERATION(args)
// - MXASIO_HANDLER_REACTOR_REGISTRATION(args)
// - MXASIO_HANDLER_REACTOR_DEREGISTRATION(args)
// - MXASIO_HANDLER_REACTOR_READ_EVENT
// - MXASIO_HANDLER_REACTOR_WRITE_EVENT
// - MXASIO_HANDLER_REACTOR_ERROR_EVENT
// - MXASIO_HANDLER_REACTOR_EVENTS(args)
// - MXASIO_HANDLER_REACTOR_OPERATION(args)

# if !defined(MXASIO_ENABLE_HANDLER_TRACKING)
#  define MXASIO_ENABLE_HANDLER_TRACKING 1
# endif /// !defined(MXASIO_ENABLE_HANDLER_TRACKING)

#elif defined(MXASIO_ENABLE_HANDLER_TRACKING)

class handler_tracking
{
public:
  class completion;

  // Base class for objects containing tracked handlers.
  class tracked_handler
  {
  private:
    // Only the handler_tracking class will have access to the id.
    friend class handler_tracking;
    friend class completion;
    uint64_t id_;

  protected:
    // Constructor initialises with no id.
    tracked_handler() : id_(0) {}

    // Prevent deletion through this type.
    ~tracked_handler() {}
  };

  // Initialise the tracking system.
  MXASIO_DECL static void init();

  class location
  {
  public:
    // Constructor adds a location to the stack.
    MXASIO_DECL explicit location(const char* file,
        int line, const char* func);

    // Destructor removes a location from the stack.
    MXASIO_DECL ~location();

  private:
    // Disallow copying and assignment.
    location(const location&) = delete;
    location& operator=(const location&) = delete;

    friend class handler_tracking;
    const char* file_;
    int line_;
    const char* func_;
    location* next_;
  };

  // Record the creation of a tracked handler.
  MXASIO_DECL static void creation(
      execution_context& context, tracked_handler& h,
      const char* object_type, void* object,
      uintmax_t native_handle, const char* op_name);

  class completion
  {
  public:
    // Constructor records that handler is to be invoked with no arguments.
    MXASIO_DECL explicit completion(const tracked_handler& h);

    // Destructor records only when an exception is thrown from the handler, or
    // if the memory is being freed without the handler having been invoked.
    MXASIO_DECL ~completion();

    // Records that handler is to be invoked with no arguments.
    MXASIO_DECL void invocation_begin();

    // Records that handler is to be invoked with one arguments.
    MXASIO_DECL void invocation_begin(const mxasio::error_code& ec);

    // Constructor records that handler is to be invoked with two arguments.
    MXASIO_DECL void invocation_begin(
        const mxasio::error_code& ec, std::size_t bytes_transferred);

    // Constructor records that handler is to be invoked with two arguments.
    MXASIO_DECL void invocation_begin(
        const mxasio::error_code& ec, int signal_number);

    // Constructor records that handler is to be invoked with two arguments.
    MXASIO_DECL void invocation_begin(
        const mxasio::error_code& ec, const char* arg);

    // Record that handler invocation has ended.
    MXASIO_DECL void invocation_end();

  private:
    friend class handler_tracking;
    uint64_t id_;
    bool invoked_;
    completion* next_;
  };

  // Record an operation that is not directly associated with a handler.
  MXASIO_DECL static void operation(execution_context& context,
      const char* object_type, void* object,
      uintmax_t native_handle, const char* op_name);

  // Record that a descriptor has been registered with the reactor.
  MXASIO_DECL static void reactor_registration(execution_context& context,
      uintmax_t native_handle, uintmax_t registration);

  // Record that a descriptor has been deregistered from the reactor.
  MXASIO_DECL static void reactor_deregistration(execution_context& context,
      uintmax_t native_handle, uintmax_t registration);

  // Record a reactor-based operation that is associated with a handler.
  MXASIO_DECL static void reactor_events(execution_context& context,
      uintmax_t registration, unsigned events);

  // Record a reactor-based operation that is associated with a handler.
  MXASIO_DECL static void reactor_operation(
      const tracked_handler& h, const char* op_name,
      const mxasio::error_code& ec);

  // Record a reactor-based operation that is associated with a handler.
  MXASIO_DECL static void reactor_operation(
      const tracked_handler& h, const char* op_name,
      const mxasio::error_code& ec, std::size_t bytes_transferred);

  // Write a line of output.
  MXASIO_DECL static void write_line(const char* format, ...);

private:
  struct tracking_state;
  MXASIO_DECL static tracking_state* get_state();
};

# define MXASIO_INHERIT_TRACKED_HANDLER \
  : public mxasio::detail::handler_tracking::tracked_handler

# define MXASIO_ALSO_INHERIT_TRACKED_HANDLER \
  , public mxasio::detail::handler_tracking::tracked_handler

# define MXASIO_HANDLER_TRACKING_INIT \
  mxasio::detail::handler_tracking::init()

# define MXASIO_HANDLER_LOCATION(args) \
  mxasio::detail::handler_tracking::location tracked_location args

# define MXASIO_HANDLER_CREATION(args) \
  mxasio::detail::handler_tracking::creation args

# define MXASIO_HANDLER_COMPLETION(args) \
  mxasio::detail::handler_tracking::completion tracked_completion args

# define MXASIO_HANDLER_INVOCATION_BEGIN(args) \
  tracked_completion.invocation_begin args

# define MXASIO_HANDLER_INVOCATION_END \
  tracked_completion.invocation_end()

# define MXASIO_HANDLER_OPERATION(args) \
  mxasio::detail::handler_tracking::operation args

# define MXASIO_HANDLER_REACTOR_REGISTRATION(args) \
  mxasio::detail::handler_tracking::reactor_registration args

# define MXASIO_HANDLER_REACTOR_DEREGISTRATION(args) \
  mxasio::detail::handler_tracking::reactor_deregistration args

# define MXASIO_HANDLER_REACTOR_READ_EVENT 1
# define MXASIO_HANDLER_REACTOR_WRITE_EVENT 2
# define MXASIO_HANDLER_REACTOR_ERROR_EVENT 4

# define MXASIO_HANDLER_REACTOR_EVENTS(args) \
  mxasio::detail::handler_tracking::reactor_events args

# define MXASIO_HANDLER_REACTOR_OPERATION(args) \
  mxasio::detail::handler_tracking::reactor_operation args

#else // defined(MXASIO_ENABLE_HANDLER_TRACKING)

# define MXASIO_INHERIT_TRACKED_HANDLER
# define MXASIO_ALSO_INHERIT_TRACKED_HANDLER
# define MXASIO_HANDLER_TRACKING_INIT (void)0
# define MXASIO_HANDLER_LOCATION(loc) (void)0
# define MXASIO_HANDLER_CREATION(args) (void)0
# define MXASIO_HANDLER_COMPLETION(args) (void)0
# define MXASIO_HANDLER_INVOCATION_BEGIN(args) (void)0
# define MXASIO_HANDLER_INVOCATION_END (void)0
# define MXASIO_HANDLER_OPERATION(args) (void)0
# define MXASIO_HANDLER_REACTOR_REGISTRATION(args) (void)0
# define MXASIO_HANDLER_REACTOR_DEREGISTRATION(args) (void)0
# define MXASIO_HANDLER_REACTOR_READ_EVENT 0
# define MXASIO_HANDLER_REACTOR_WRITE_EVENT 0
# define MXASIO_HANDLER_REACTOR_ERROR_EVENT 0
# define MXASIO_HANDLER_REACTOR_EVENTS(args) (void)0
# define MXASIO_HANDLER_REACTOR_OPERATION(args) (void)0

#endif // defined(MXASIO_ENABLE_HANDLER_TRACKING)

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#if defined(MXASIO_HEADER_ONLY)
#include "mxasio/detail/impl/handler_tracking.ipp"
#endif // defined(MXASIO_HEADER_ONLY)

#endif // MXASIO_DETAIL_HANDLER_TRACKING_HPP
