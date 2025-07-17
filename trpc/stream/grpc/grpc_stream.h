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

#include <memory>

#include "trpc/codec/grpc/http2/session.h"
#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/filter/filter_controller.h"
#include "trpc/stream/common_stream.h"

namespace trpc::stream {

/// @brief Implements the specific interaction logic between business and gRPC streams, as well as the specific
/// interaction logic between transport layer and gRPC streams.
class GrpcStream : public CommonStream {
 public:
  explicit GrpcStream(StreamOptions&& options, http2::Session* session, FiberMutex* mutex, FiberConditionVariable* cv)
      : CommonStream(std::move(options)), session_(session), mutex_(mutex), cv_(cv) {}
  ~GrpcStream() = default;

  /// @brief Closes the stream if RPC call finished.
  void Close(Status status) override;

  /// @brief Resets the stream if an error occurred.
  void Reset(Status status) override;

  /// @brief Returns the code of encoding error.
  int GetEncodeErrorCode() override;

  /// @brief Returns the code of decoding error.
  int GetDecodeErrorCode() override;

  /// @brief Returns the code of timeout error.
  int GetReadTimeoutErrorCode() override;

  /// @brief Handles stream error.
  void OnError(Status status) { CommonStream::OnError(status); }

 protected:
  RetCode HandleInit(StreamRecvMessage&& msg) override { return RetCode::kError; }

  RetCode HandleData(StreamRecvMessage&& msg) override { return RetCode::kError; }

  RetCode HandleFeedback(StreamRecvMessage&& msg) override {
    // gRPC has implemented stream control frame processing in the nghttp2 library, so there is no need to
    // handle the corresponding frames in the upper layer.
    return RetCode::kError;
  }

  RetCode HandleClose(StreamRecvMessage&& msg) override { return RetCode::kError; }

  RetCode HandleReset(StreamRecvMessage&& msg) override;

  RetCode SendInit(StreamSendMessage&& msg) override { return RetCode::kError; }

  RetCode SendData(StreamSendMessage&& msg) override { return RetCode::kError; }

  RetCode SendFeedback(StreamSendMessage&& msg) override {
    // gRPC has implemented stream control frame processing in the nghttp2 library, so there is no need to
    // handle the corresponding frames in the upper layer.
    return RetCode::kError;
  }

  RetCode SendClose(StreamSendMessage&& msg) override { return RetCode::kError; }

  RetCode SendReset(StreamSendMessage&& msg) override;

 protected:
  // @brief The encoding type for sending stream messages.
  enum class EncodeMessageType { kHeader = 0, kData, kTrailer, kReset };

 protected:
  // @brief Encode message.
  // @param type Encoding type.
  // @param[in] msg Unencoded message.
  // @param[out] buffer Encoded message.
  bool EncodeMessage(EncodeMessageType type, std::any&& msg, NoncontiguousBuffer* buffer);

  // @brief When there is an error in the stream, send RESET directly to terminate the stream.
  virtual void SendReset(uint32_t error_code);

 protected:
  // Save session/fiber locks to facilitate encoding/decoding and flow control strategies. We use raw pointers here
  // because `session_` will be attached to the `stream_handler`. Once the `stream` is removed from it, it will no
  // longer access `session_`.
  http2::Session* session_{nullptr};
  FiberMutex* mutex_{nullptr};
  // Used to implement connection-level flow control.
  FiberConditionVariable* cv_{nullptr};
};

using GrpcStreamPtr = RefPtr<GrpcStream>;

}  // namespace trpc::stream
