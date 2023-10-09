//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include <optional>
#include <string>

#include "trpc/common/status.h"
#include "trpc/stream/stream_provider.h"

namespace trpc::stream {

/// @brief Definition of the return status for stream reader/writer operations.
/// @note Maintaining the current design for the sake of compatibility. However, it may be worth considering a redesign
///       of the error handling system in the future, including changes to usage and naming conventions.
const Status kStreamStatusClientReadTimeout{TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR, 0, "read timeout"};
const Status kStreamStatusServerReadTimeout{TRPC_STREAM_SERVER_READ_TIMEOUT_ERR, 0, "read timeout"};
const Status kStreamStatusClientWriteTimeout{TRPC_STREAM_CLIENT_WRITE_TIMEOUT_ERR, 0, "write timeout"};
const Status kStreamStatusServerWriteTimeout{TRPC_STREAM_SERVER_WRITE_TIMEOUT_ERR, 0, "write timeout"};
const Status kStreamStatusClientWriteContentLengthError{TRPC_STREAM_CLIENT_WRITE_OVERFLOW_ERR, 0,
                                                        "write length not equal to Content-Length"};
const Status kStreamStatusServerWriteContentLengthError{TRPC_STREAM_SERVER_WRITE_OVERFLOW_ERR, 0,
                                                        "write length not equal to Content-Length"};
const Status kStreamStatusClientNetworkError{TRPC_STREAM_CLIENT_NETWORK_ERR, 0, "network error"};
const Status kStreamStatusServerNetworkError{TRPC_STREAM_SERVER_NETWORK_ERR, 0, "network error"};
const Status kStreamStatusReadEof{static_cast<int>(StreamStatus::kStreamEof), 0, "read eof"};
const Status kStreamStatusServerMessageExceedLimit{TRPC_STREAM_SERVER_MSG_EXCEED_LIMIT_ERR, 0, "packet too large"};

/// @brief Generates the http chunk length field.
/// @private For internal use purpose only.
std::string HttpChunkHeader(size_t chunk_length);

/// @brief internal implementations for stream sub-module.
/// @private For internal use purpose only.
namespace detail {

/// @private For internal use purpose only.
template <typename T>
class TransientWrapper : public std::optional<T> {
 public:
  constexpr TransientWrapper() noexcept = default;

  template <typename... Args>
  constexpr explicit TransientWrapper(Args&&... args) noexcept
      : std::optional<T>(std::in_place, std::forward<Args>(args)...) {}

  constexpr TransientWrapper(const TransientWrapper&) noexcept {}

  constexpr TransientWrapper(TransientWrapper&&) noexcept {}

  constexpr TransientWrapper& operator=(const TransientWrapper&) noexcept { return *this; }

  constexpr TransientWrapper& operator=(TransientWrapper&&) noexcept { return *this; }
};

}  // namespace detail

}  // namespace trpc::stream
