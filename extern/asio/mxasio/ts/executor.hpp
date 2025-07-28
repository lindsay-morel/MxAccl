//
// ts/executor.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_TS_EXECUTOR_HPP
#define MXASIO_TS_EXECUTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/async_result.hpp"
#include "mxasio/associated_allocator.hpp"
#include "mxasio/execution_context.hpp"
#include "mxasio/is_executor.hpp"
#include "mxasio/associated_executor.hpp"
#include "mxasio/bind_executor.hpp"
#include "mxasio/executor_work_guard.hpp"
#include "mxasio/system_executor.hpp"
#include "mxasio/executor.hpp"
#include "mxasio/any_io_executor.hpp"
#include "mxasio/dispatch.hpp"
#include "mxasio/post.hpp"
#include "mxasio/defer.hpp"
#include "mxasio/strand.hpp"
#include "mxasio/packaged_task.hpp"
#include "mxasio/use_future.hpp"

#endif // MXASIO_TS_EXECUTOR_HPP
