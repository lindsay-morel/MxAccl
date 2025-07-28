//
// detail/impl/buffer_sequence_adapter.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MXASIO_DETAIL_IMPL_BUFFER_SEQUENCE_ADAPTER_IPP
#define MXASIO_DETAIL_IMPL_BUFFER_SEQUENCE_ADAPTER_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "mxasio/detail/config.hpp"

#if defined(MXASIO_WINDOWS_RUNTIME)

#include <robuffer.h>
#include <windows.storage.streams.h>
#include <wrl/implements.h>
#include "mxasio/detail/buffer_sequence_adapter.hpp"

#include "mxasio/detail/push_options.hpp"

namespace mxasio {
namespace detail {

class winrt_buffer_impl :
  public Microsoft::WRL::RuntimeClass<
    Microsoft::WRL::RuntimeClassFlags<
      Microsoft::WRL::RuntimeClassType::WinRtClassicComMix>,
    ABI::Windows::Storage::Streams::IBuffer,
    Windows::Storage::Streams::IBufferByteAccess>
{
public:
  explicit winrt_buffer_impl(const mxasio::const_buffer& b)
  {
    bytes_ = const_cast<byte*>(static_cast<const byte*>(b.data()));
    length_ = b.size();
    capacity_ = b.size();
  }

  explicit winrt_buffer_impl(const mxasio::mutable_buffer& b)
  {
    bytes_ = static_cast<byte*>(b.data());
    length_ = 0;
    capacity_ = b.size();
  }

  ~winrt_buffer_impl()
  {
  }

  STDMETHODIMP Buffer(byte** value)
  {
    *value = bytes_;
    return S_OK;
  }

  STDMETHODIMP get_Capacity(UINT32* value)
  {
    *value = capacity_;
    return S_OK;
  }

  STDMETHODIMP get_Length(UINT32 *value)
  {
    *value = length_;
    return S_OK;
  }

  STDMETHODIMP put_Length(UINT32 value)
  {
    if (value > capacity_)
      return E_INVALIDARG;
    length_ = value;
    return S_OK;
  }

private:
  byte* bytes_;
  UINT32 length_;
  UINT32 capacity_;
};

void buffer_sequence_adapter_base::init_native_buffer(
    buffer_sequence_adapter_base::native_buffer_type& buf,
    const mxasio::mutable_buffer& buffer)
{
  std::memset(&buf, 0, sizeof(native_buffer_type));
  Microsoft::WRL::ComPtr<IInspectable> insp
    = Microsoft::WRL::Make<winrt_buffer_impl>(buffer);
  buf = reinterpret_cast<Windows::Storage::Streams::IBuffer^>(insp.Get());
}

void buffer_sequence_adapter_base::init_native_buffer(
    buffer_sequence_adapter_base::native_buffer_type& buf,
    const mxasio::const_buffer& buffer)
{
  std::memset(&buf, 0, sizeof(native_buffer_type));
  Microsoft::WRL::ComPtr<IInspectable> insp
    = Microsoft::WRL::Make<winrt_buffer_impl>(buffer);
  Platform::Object^ buf_obj = reinterpret_cast<Platform::Object^>(insp.Get());
  buf = reinterpret_cast<Windows::Storage::Streams::IBuffer^>(insp.Get());
}

} // namespace detail
} // namespace mxasio

#include "mxasio/detail/pop_options.hpp"

#endif // defined(MXASIO_WINDOWS_RUNTIME)

#endif // MXASIO_DETAIL_IMPL_BUFFER_SEQUENCE_ADAPTER_IPP
