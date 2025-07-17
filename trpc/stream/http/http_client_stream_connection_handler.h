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
#include "trpc/stream/stream_handler.h"
#include "trpc/transport/client/fiber/common/fiber_client_connection_handler.h"

namespace trpc::stream {

/// @brief Implementation of stream connection handler for HTTP client stream which works in FIBER thread model.
class FiberHttpClientStreamConnectionHandler : public FiberClientStreamConnectionHandler {
 public:
  explicit FiberHttpClientStreamConnectionHandler(Connection* conn, TransInfo* trans_info)
      : FiberClientStreamConnectionHandler(conn, trans_info) {}

  ~FiberHttpClientStreamConnectionHandler() override;

  void Init() override;
  int CheckMessage(const ConnectionPtr&, NoncontiguousBuffer&, std::deque<std::any>&) override;
  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& msg) override;
  StreamHandlerPtr GetOrCreateStreamHandler() override;
  void CleanResource() override;
  void Stop() override {
    if (stream_handler_) {
      stream_handler_->Stop();
    }
  }
  void Join() override {
    if (stream_handler_) {
      stream_handler_->Join();
    }
  }

 private:
  StreamHandlerPtr stream_handler_{nullptr};
  FiberMutex mutex_;
};

}  // namespace trpc::stream
