//
// detail/win_event.ipp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_IMPL_WIN_EVENT_IPP
#define MXASIO_DETAIL_IMPL_WIN_EVENT_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_WINDOWS)

#include "mxasio/detail/throw_error.hpp"
#include "mxasio/detail/win_event.hpp"
#include "mxasio/error.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

win_event::win_event()
  : state_(0)
{
#if defined(MXASIO_WINDOWS_APP)
  events_[0] = ::CreateEventExW(0, 0,
      CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
#else // defined(MXASIO_WINDOWS_APP)
  events_[0] = ::CreateEventW(0, true, false, 0);
#endif // defined(MXASIO_WINDOWS_APP)
  if (!events_[0])
  {
    DWORD last_error = ::GetLastError();
    mxasio::error_code ec(last_error,
        mxasio::error::get_system_category());
    mxasio::detail::throw_error(ec, "event");
  }

#if defined(MXASIO_WINDOWS_APP)
  events_[1] = ::CreateEventExW(0, 0, 0, EVENT_ALL_ACCESS);
#else // defined(MXASIO_WINDOWS_APP)
  events_[1] = ::CreateEventW(0, false, false, 0);
#endif // defined(MXASIO_WINDOWS_APP)
  if (!events_[1])
  {
    DWORD last_error = ::GetLastError();
    ::CloseHandle(events_[0]);
    mxasio::error_code ec(last_error,
        mxasio::error::get_system_category());
    mxasio::detail::throw_error(ec, "event");
  }
}

win_event::~win_event()
{
  ::CloseHandle(events_[0]);
  ::CloseHandle(events_[1]);
}

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // defined(MXASIO_WINDOWS)

#endif // MXASIO_DETAIL_IMPL_WIN_EVENT_IPP
