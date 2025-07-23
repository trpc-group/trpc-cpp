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

#include "trpc/stream/http/common/stream.h"
#include "trpc/stream/stream_handler.h"

namespace trpc::stream {

/// @brief The basic class for http StreamHandler. It Implements the common logic of http stream handler, includes
///        RemoveStream, PushMessage, SendMessage and ParseMessage interfaces. For ParseMessage, it only implements the
///        parsing logic with the common msg types.
class HttpCommonStreamHandler : public StreamHandler {
 public:
  explicit HttpCommonStreamHandler(StreamOptions&& options) : options_(std::move(options)) {}

  StreamOptions* GetMutableStreamOptions() override { return &options_; }

  bool Init() override { return true; }

  void Stop() override;

  StreamReaderWriterProviderPtr CreateStream(StreamOptions&& options) override {
    TRPC_ASSERT(false && "Should Implement");
  }

  bool IsNewStream(uint32_t stream_id, uint32_t frame_type) override;

  int RemoveStream(uint32_t stream_id) override;

  void PushMessage(std::any&& message, ProtocolMessageMetadata&& metadata) override;

  int SendMessage(const std::any& msg, NoncontiguousBuffer&& send_data) override;

  /// @brief Parses the network data received.
  /// @note It calls ParseMessageImpl to perform the actual parsing logic. Subclasses need to override ParseMessageImpl
  ///       instead of ParseMessage.
  int ParseMessage(NoncontiguousBuffer* in, std::deque<std::any>* out) override;

 protected:
  /// @brief The types of parsing states.
  enum class ParseState {
    kIdle,
    kAfterParseStartLine,
    kAfterParseHeader,
    kAfterParseData,
    kFinish,
  };

 protected:
  /// @brief Implements the actual parsing logic.
  /// @return Return kParseHttpSucc on success, kParseHttpNeedMore on insufficient message, kParseHttpError on error
  /// @note It implements the common parsing logic for both requests and responses, and subclasses need to implement the
  ///       logic for their own specific message types.
  virtual int ParseMessageImpl(NoncontiguousBuffer* in, std::deque<std::any>* out);

 protected:
  /// @brief The stream mounts on the connection, where currently only one stream can be mounted on a single connection
  HttpCommonStreamPtr stream_{nullptr};

  /// @brief The flag indicating whether the connection is closed
  bool conn_closed_{false};

  /// @brief The parse state
  ParseState parse_state_{ParseState::kIdle};
  /// @brief The flag indicating whether is chunked
  bool is_chunked_{false};
  /// @brief The bytes amount expect to received
  size_t expect_bytes_{0};
  /// @brief The flag indicating whether has trailer
  bool has_trailer_{false};

  /// @brief The flag indicating whether pass the idle state successfully
  bool pass_state_idle_{false};
  /// @brief The flag indicating whether finish parsing successfully
  bool pass_state_finish_{false};

 private:
  StreamOptions options_;

  /// @brief Cache the parsed results of ParseMessageImpl
  std::deque<std::any> cached_out_;
};

}  // namespace trpc::stream
