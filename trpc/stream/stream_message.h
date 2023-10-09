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

#include <any>
#include <string>

namespace trpc::stream {

/// @brief Return code of message processing.
enum RetCode {
  // Processes message successfully.
  kSuccess = 0,
  // Failed to process message.
  kError,
};

/// @brief stream message category, currently enumerated tRPC protocol stream frame types, other protocols can
/// be expanded as needed.
enum class StreamMessageCategory {
  // Initializes a stream.
  kStreamInit = 1,
  // stream data payload.
  kStreamData = 2,
  // Provides a feedback of stream state, such as send-received window size.
  kStreamFeedback = 3,
  // Closes the stream.
  kStreamClose = 4,
  // Finishes the stream RPC.
  kStreamFinish = 5,
  // Reset the stream.
  kStreamReset = 6,
  // Unsupported frame.
  kStreamUnknown = 99,
};

/// @brief Protocol message metadata.
struct ProtocolMessageMetadata {
  // Protocol packet type, such as UNARY RPC, STREAM RPC.
  uint8_t data_frame_type{0};
  // Flag indicating whether to support streaming RPC
  bool enable_stream{false};
  // Streaming data frame type, such as INIT_FRAME, DATA_FRAME, CLOSE_FRAME, FEEDBACK_FRAME.
  uint8_t stream_frame_type{0};
  // Stream identifier.
  uint32_t stream_id{0};
};

/// @brief Stream receiving message, including message content and metadata.
struct StreamRecvMessage {
  // Message content.
  std::any message{nullptr};
  // Message metadata.
  ProtocolMessageMetadata metadata;
};

/// @brief Stream sending message, including message category and message data.
struct StreamSendMessage {
  // Message category.
  StreamMessageCategory category{};
  // Message content.
  std::any data_provider{nullptr};
  // Message metadata.
  std::any metadata{nullptr};
};

}  // namespace trpc::stream
