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

#include "trpc/stream/http/async/stream.h"
#include "trpc/util/http/request.h"

namespace trpc::stream {

/// @brief The server asynchronous stream of http
class HttpServerAsyncStream : public HttpAsyncStream {
 public:
  explicit HttpServerAsyncStream(StreamOptions&& options) : HttpAsyncStream(std::move(options)) {}

  /// @brief Gets the request line
  const HttpRequestLine& GetRequestLine();

  /// @brief Gets the full request if the request has arrived completely. In this case, waiting for the data to arrive
  ///        through the future-then process is not necessary, which can optimize performance in unary scenarios.
  /// @note It must be used in conjunction with the HasFullMessage interface.
  http::RequestPtr GetFullRequest();

  /// @brief Gets the path parameters (Placeholder in URL path).
  const http::PathParameters& GetParameters() const;
  http::PathParameters* GetMutableParameters();

  /// @brief Sets the path parameters (Placeholder in URL path).
  void SetParameters(http::Parameters&& param);

  void Reset(Status status = kUnknownErrorStatus) override;

 protected:
  int GetReadTimeoutErrorCode() override { return TrpcRetCode::TRPC_STREAM_SERVER_READ_TIMEOUT_ERR; }

  int GetWriteTimeoutErrorCode() override { return TrpcRetCode::TRPC_STREAM_SERVER_WRITE_TIMEOUT_ERR; }

  int GetNetWorkErrorCode() override { return TrpcRetCode::TRPC_STREAM_SERVER_NETWORK_ERR; }

  /// @brief Handles the received stream frames. It mainly implements the processing of request line frames, and for
  ///        other types, it directly calls the implementation of the parent class.
  RetCode HandleRecvMessage(HttpStreamFramePtr&& msg) override;

  /// @brief Handles the stream frames to be sent. It mainly implements the processing of request line frames, and for
  ///        other types, it directly calls the implementation of the parent class.
  RetCode HandleSendMessage(HttpStreamFramePtr&& msg, NoncontiguousBuffer* out) override;

  void OnError(Status status) override;

 private:
  // Handles the request line received
  RetCode HandleRequestLine(HttpRequestLine* start_line);
  // Performs before sending the request line
  RetCode PreSendStatusLine(HttpStatusLine* status_line, NoncontiguousBuffer* out);

 private:
  // request line
  HttpRequestLine start_line_;
  http::PathParameters path_parameters_;
};
using HttpServerAsyncStreamPtr = RefPtr<HttpServerAsyncStream>;

}  // namespace trpc::stream
