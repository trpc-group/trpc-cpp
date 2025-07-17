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

#include "trpc/stream/http/http_client_stream_handler.h"

namespace trpc::stream {

HttpClientStreamHandler::~HttpClientStreamHandler() noexcept { stream_ = nullptr; }

StreamReaderWriterProviderPtr HttpClientStreamHandler::CreateStream(StreamOptions&& options) {
  stream_ = MakeRefCounted<HttpClientStream>(std::move(options));
  stream_->SetFilterController(&filter_controller_);
  return stream_;
}

int HttpClientStreamHandler::SendMessage(const std::any& msg, NoncontiguousBuffer&& send_data) {
  IoMessage io_message;
  io_message.buffer = std::move(send_data);
  io_message.msg = msg;
  return options_.send(std::move(io_message));
}

void HttpClientStreamHandler::Stop() {
  std::unique_lock lock(mutex_);
  if (stream_) {
    stream_->Close();
  }
}

int HttpClientStreamHandler::RemoveStream(uint32_t stream_id) {
  std::unique_lock lock(mutex_);
  stream_ = nullptr;
  return 0;
}

void HttpClientStreamHandler::PushMessage(std::any&& message, ProtocolMessageMetadata&& metadata) {
  // Only the HTTP response header information will enter this process. It is expected to be called only once during
  // the entire stream process, and the locking does not affect performance.
  std::unique_lock lock(mutex_);
  if (stream_) {
    stream_->PushRecvMessage(std::move(message));
  }
}

}  // namespace trpc::stream
