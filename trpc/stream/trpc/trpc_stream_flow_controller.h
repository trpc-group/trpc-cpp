//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include "trpc/util/ref_ptr.h"

namespace trpc::stream {

// @brief Default window size fow flow control.
constexpr uint32_t kTrpcStreamDefaultWindowSize = 65535;

/// @brief Get the flow control window size. If it is less than 65535, it is considered an unreasonable window setting
/// and will be set to 65535 (except for 0, flow control is enabled by default, 0 means that the business needs to
/// turn off flow control, and 0 can also be considered as an infinitely large flow control window).
inline uint32_t GetTrpcStreamWindowSize(uint32_t window_size) {
  if (window_size == 0) {
    return window_size;
  }
  if (window_size < kTrpcStreamDefaultWindowSize) {
    return kTrpcStreamDefaultWindowSize;
  }
  return window_size;
}

/// @brief Implementation of flow control for sending flow control.
class TrpcStreamSendController : public RefCounted<TrpcStreamSendController> {
 public:
  explicit TrpcStreamSendController(uint32_t window) : window_(window) {}

  /// @brief When sending data, the sending window needs to be reduced.
  /// @param decrement The size by which the sending window needs to be reduced.
  /// @return Returns true if there is enough flow control window (the window value is reduced), and false if the flow
  /// control window is insufficient (the window value remains unchanged).
  bool DecreaseWindow(uint32_t decrement) {
    if (window_ < 0) {
      return false;
    }
    window_ -= static_cast<int64_t>(decrement);
    return true;
  }

  /// @brief After receiving feedback, increase the sending window size.
  /// @param increment The size by which the sending window needs to be increased.
  /// @return Returns true if sending can be performed (the window value is increased to greater than 0), and false
  /// if sending cannot be performed (the window value remains less than or equal to 0 after being increased).
  bool IncreaseWindow(uint32_t increment) {
    window_ += static_cast<int64_t>(increment);
    return window_ > 0;
  }

 private:
  int64_t window_{0};
};

/// @brief Implementation of flow control for receiving flow control.
class TrpcStreamRecvController : public RefCounted<TrpcStreamRecvController> {
 public:
  explicit TrpcStreamRecvController(uint32_t window) : window_(window) {}

  /// @brief Called when reading data from the stream, when the length consumed exceeds 1/4 of the window value,
  /// the caller needs to send feedback.
  /// @return 0 means no need to send feedback, >0 means the window value carried in the feedback that needs to be sent.
  int UpdateConsumeBytes(uint32_t consume_bytes) {
    consumed_ += consume_bytes;
    if (consumed_ > window_ / 4) {
      uint32_t consumed = consumed_;
      consumed_ = 0;
      return consumed;
    }
    return 0;
  }

 private:
  uint32_t window_{0};
  uint32_t consumed_{0};
};

}  // namespace trpc::stream
