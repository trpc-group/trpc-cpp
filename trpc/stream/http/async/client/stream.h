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

#include "trpc/client/client_context.h"
#include "trpc/filter/client_filter_controller.h"
#include "trpc/stream/http/async/stream.h"

namespace trpc::stream {

/// @brief The asynchronous HTTP client stream.
class HttpClientAsyncStream : public HttpAsyncStream {
 public:
  explicit HttpClientAsyncStream(StreamOptions&& options) : HttpAsyncStream(std::move(options)) {}

  /// @brief Reads the status line of response.
  Future<HttpStatusLine> ReadStatusLine(int timeout = std::numeric_limits<int>::max());

  /// @brief Reset the connection when error occurred.
  void Reset(Status status) override;

  /// @brief Sets the pointcut executor, one for each proxy, to execute post-pointcut when the stream fails or ends.
  /// @note The server does not need it because its pointcut execution has been embedded in the main framework service
  /// process, but the client call completion is not sensed by the service layer, so its post-pointcut needs to be
  /// executed when `Finish` is called in the stream.
  void SetFilterController(ClientFilterController* filter_controller) { filter_controller_ = filter_controller; }

  /// @brief The method that must be called last in the client downstream usage, which will execute the post-pointcut
  /// of the interceptor and destroy the resources of the stream.
  Status Finish() override;

 protected:
  int GetReadTimeoutErrorCode() override { return TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR; }

  int GetWriteTimeoutErrorCode() override { return TRPC_STREAM_CLIENT_WRITE_TIMEOUT_ERR; }

  int GetNetWorkErrorCode() override { return TRPC_STREAM_CLIENT_NETWORK_ERR; }

 protected:
  /// @brief Handles the status line of response.
  RetCode HandleRecvMessage(HttpStreamFramePtr&& msg) override;

  /// @brief Sends the request line.
  RetCode HandleSendMessage(HttpStreamFramePtr&& msg, NoncontiguousBuffer* out) override;

  void OnError(Status status) override;

 private:
  RetCode HandleStatusLine(HttpStatusLine* status_line);

  RetCode PreSendRequestLine(HttpRequestLine* start_line, NoncontiguousBuffer* out);

 private:
  std::optional<HttpStatusLine> status_line_;
  PendingVal<HttpStatusLine> pending_status_line_;
  ClientFilterController* filter_controller_{nullptr};
};
using HttpClientAsyncStreamPtr = RefPtr<HttpClientAsyncStream>;

}  // namespace trpc::stream
