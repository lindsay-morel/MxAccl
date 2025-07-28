//
// impl/src.hpp
// ~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_IMPL_SRC_HPP
#define MXASIO_IMPL_SRC_HPP

#define MXASIO_SOURCE

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_HEADER_ONLY)
# error Do not compile Asio library source with MXASIO_HEADER_ONLY defined
#endif

#include "mxasio/impl/any_completion_executor.ipp"
#include "mxasio/impl/any_io_executor.ipp"
#include "mxasio/impl/cancellation_signal.ipp"
#include "mxasio/impl/connect_pipe.ipp"
#include "mxasio/impl/error.ipp"
#include "mxasio/impl/error_code.ipp"
#include "mxasio/impl/execution_context.ipp"
#include "mxasio/impl/executor.ipp"
#include "mxasio/impl/io_context.ipp"
#include "mxasio/impl/multiple_exceptions.ipp"
#include "mxasio/impl/serial_port_base.ipp"
#include "mxasio/impl/system_context.ipp"
#include "mxasio/impl/thread_pool.ipp"
#include "mxasio/detail/impl/buffer_sequence_adapter.ipp"
#include "mxasio/detail/impl/descriptor_ops.ipp"
#include "mxasio/detail/impl/dev_poll_reactor.ipp"
#include "mxasio/detail/impl/epoll_reactor.ipp"
#include "mxasio/detail/impl/eventfd_select_interrupter.ipp"
#include "mxasio/detail/impl/handler_tracking.ipp"
#include "mxasio/detail/impl/io_uring_descriptor_service.ipp"
#include "mxasio/detail/impl/io_uring_file_service.ipp"
#include "mxasio/detail/impl/io_uring_socket_service_base.ipp"
#include "mxasio/detail/impl/io_uring_service.ipp"
#include "mxasio/detail/impl/kqueue_reactor.ipp"
#include "mxasio/detail/impl/null_event.ipp"
#include "mxasio/detail/impl/pipe_select_interrupter.ipp"
#include "mxasio/detail/impl/posix_event.ipp"
#include "mxasio/detail/impl/posix_mutex.ipp"
#include "mxasio/detail/impl/posix_serial_port_service.ipp"
#include "mxasio/detail/impl/posix_thread.ipp"
#include "mxasio/detail/impl/posix_tss_ptr.ipp"
#include "mxasio/detail/impl/reactive_descriptor_service.ipp"
#include "mxasio/detail/impl/reactive_socket_service_base.ipp"
#include "mxasio/detail/impl/resolver_service_base.ipp"
#include "mxasio/detail/impl/scheduler.ipp"
#include "mxasio/detail/impl/select_reactor.ipp"
#include "mxasio/detail/impl/service_registry.ipp"
#include "mxasio/detail/impl/signal_set_service.ipp"
#include "mxasio/detail/impl/socket_ops.ipp"
#include "mxasio/detail/impl/socket_select_interrupter.ipp"
#include "mxasio/detail/impl/strand_executor_service.ipp"
#include "mxasio/detail/impl/strand_service.ipp"
#include "mxasio/detail/impl/thread_context.ipp"
#include "mxasio/detail/impl/throw_error.ipp"
#include "mxasio/detail/impl/timer_queue_ptime.ipp"
#include "mxasio/detail/impl/timer_queue_set.ipp"
#include "mxasio/detail/impl/win_iocp_file_service.ipp"
#include "mxasio/detail/impl/win_iocp_handle_service.ipp"
#include "mxasio/detail/impl/win_iocp_io_context.ipp"
#include "mxasio/detail/impl/win_iocp_serial_port_service.ipp"
#include "mxasio/detail/impl/win_iocp_socket_service_base.ipp"
#include "mxasio/detail/impl/win_event.ipp"
#include "mxasio/detail/impl/win_mutex.ipp"
#include "mxasio/detail/impl/win_object_handle_service.ipp"
#include "mxasio/detail/impl/win_static_mutex.ipp"
#include "mxasio/detail/impl/win_thread.ipp"
#include "mxasio/detail/impl/win_tss_ptr.ipp"
#include "mxasio/detail/impl/winrt_ssocket_service_base.ipp"
#include "mxasio/detail/impl/winrt_timer_scheduler.ipp"
#include "mxasio/detail/impl/winsock_init.ipp"
#include "mxasio/execution/impl/bad_executor.ipp"
#include "mxasio/experimental/impl/channel_error.ipp"
#include "mxasio/generic/detail/impl/endpoint.ipp"
#include "mxasio/ip/impl/address.ipp"
#include "mxasio/ip/impl/address_v4.ipp"
#include "mxasio/ip/impl/address_v6.ipp"
#include "mxasio/ip/impl/host_name.ipp"
#include "mxasio/ip/impl/network_v4.ipp"
#include "mxasio/ip/impl/network_v6.ipp"
#include "mxasio/ip/detail/impl/endpoint.ipp"
#include "mxasio/local/detail/impl/endpoint.ipp"

#endif // MXASIO_IMPL_SRC_HPP
