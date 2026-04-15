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

#include "trpc/codec/http/http_protocol.h"
#include "trpc/util/http/sse/sse_parser.h"

namespace trpc {

/// @brief The codec protocol name for HTTP SSE.
constexpr char kHttpSseCodecName[] = "http_sse";

/// @brief HTTP SSE request protocol message.
class HttpSseRequestProtocol : public HttpRequestProtocol {
 public:
  HttpSseRequestProtocol() : HttpRequestProtocol() {}
  explicit HttpSseRequestProtocol(http::RequestPtr&& request) : HttpRequestProtocol(std::move(request)) {}
  ~HttpSseRequestProtocol() override = default;

  /// @brief Get the SSE event from the request body.
  /// @return Returns the parsed SSE event if the request body contains valid SSE data.
  std::optional<http::sse::SseEvent> GetSseEvent() const;

  /// @brief Set the SSE event as the request body.
  /// @param event The SSE event to set.
  void SetSseEvent(const http::sse::SseEvent& event);
};

/// @brief HTTP SSE response protocol message.
class HttpSseResponseProtocol : public HttpResponseProtocol {
 public:
  HttpSseResponseProtocol() = default;
  ~HttpSseResponseProtocol() override = default;

  /// @brief Get the SSE event from the response body.
  /// @return Returns the parsed SSE event if the response body contains valid SSE data.
  std::optional<http::sse::SseEvent> GetSseEvent() const;

  /// @brief Set the SSE event as the response body.
  /// @param event The SSE event to set.
  void SetSseEvent(const http::sse::SseEvent& event);

  /// @brief Set multiple SSE events as the response body.
  /// @param events The vector of SSE events to set.
  void SetSseEvents(const std::vector<http::sse::SseEvent>& events);
};

}  // namespace trpc
