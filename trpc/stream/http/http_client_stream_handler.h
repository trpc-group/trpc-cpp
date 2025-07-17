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

#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/filter/client_filter_controller.h"
#include "trpc/stream/http/http_client_stream.h"
#include "trpc/stream/stream_handler.h"
#include "trpc/stream/stream_message.h"

namespace trpc::stream {

/// @brief The implementation of HTTP client stream handler.
class HttpClientStreamHandler : public StreamHandler {
 public:
  explicit HttpClientStreamHandler(StreamOptions&& options) : options_(std::move(options)) {}

  ~HttpClientStreamHandler() override;

  StreamReaderWriterProviderPtr CreateStream(StreamOptions&& options) override;

  int SendMessage(const std::any& msg, NoncontiguousBuffer&& send_data) override;

  bool Init() override { return true; }

  void Stop() override;

  int RemoveStream(uint32_t stream_id) override;

  bool IsNewStream(uint32_t stream_id, uint32_t frame_type) override { return false; }

  void PushMessage(std::any&& message, ProtocolMessageMetadata&& metadata) override;

  HttpClientStreamPtr& GetHttpStream() { return stream_; }

 private:
  StreamOptions options_;
  // Only one stream mounted on the connection.
  HttpClientStreamPtr stream_;
  FiberMutex mutex_;
  ClientFilterController filter_controller_;
};

using HttpClientStreamHandlerPtr = RefPtr<HttpClientStreamHandler>;

}  // namespace trpc::stream
