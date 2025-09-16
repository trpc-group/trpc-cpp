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

#include <string>
#include <functional>

#include "trpc/client/client_context.h"
#include "trpc/client/service_proxy.h"
#include "trpc/client/sse/http_sse_stream_reader.h"
#include "trpc/codec/http/http_protocol.h"
#include "trpc/stream/stream_provider.h"
#include "trpc/util/http/sse/sse_event.h"

namespace trpc {

/// @brief HTTP SSE service proxy for creating SSE stream connections.
class HttpSseProxy : public ServiceProxy {
 public:
  /// @brief Event callback function type for SSE events
  using EventCallback = std::function<void(const http::sse::SseEvent&)>;

  /// @brief Creates an HTTP SSE stream reader for receiving Server-Sent Events.
  /// @param ctx Client context containing request metadata.
  /// @param url The URL to connect to for SSE events.
  /// @return HttpSseStreamReader for reading SSE events.
  HttpSseStreamReader Get(const ClientContextPtr& ctx, const std::string& url);

  /// @brief Creates an HTTP SSE stream reader for receiving Server-Sent Events with custom HTTP method.
  /// @param ctx Client context containing request metadata.
  /// @param url The URL to connect to for SSE events.
  /// @param method HTTP method to use (typically GET for SSE).
  /// @return HttpSseStreamReader for reading SSE events.
  HttpSseStreamReader Get(const ClientContextPtr& ctx, const std::string& url, const std::string& method);

  /// @brief Starts an SSE client in a background thread that automatically reads events and dispatches them to a callback.
  /// @param ctx Client context containing request metadata.
  /// @param url The URL to connect to for SSE events.
  /// @param cb Callback function to be invoked for each received SSE event.
  /// @return true if the background thread was started successfully, false otherwise.
  bool Start(const ClientContextPtr& ctx, const std::string& url, EventCallback cb);

  /// @brief Starts an SSE client in a background thread that automatically reads events and dispatches them to a callback.
  /// @param ctx Client context containing request metadata.
  /// @param url The URL to connect to for SSE events.
  /// @param method HTTP method to use (typically GET for SSE).
  /// @param cb Callback function to be invoked for each received SSE event.
  /// @return true if the background thread was started successfully, false otherwise.
  bool Start(const ClientContextPtr& ctx, const std::string& url, const std::string& method, EventCallback cb);

 private:
  /// @brief Creates the stream reader implementation.
  /// @param ctx Client context containing request metadata.
  /// @param url The URL to connect to for SSE events.
  /// @param method HTTP method to use.
  /// @return HttpSseStreamReader for reading SSE events.
  HttpSseStreamReader CreateStreamReader(const ClientContextPtr& ctx, const std::string& url, 
                                         const std::string& method);

  /// @brief Builds the HTTP request header with SSE-specific settings.
  /// @param ctx Client context containing request metadata.
  /// @param url The URL to connect to for SSE events.
  /// @param method HTTP method to use.
  /// @param req The HTTP request protocol to populate.
  void BuildRequestHeader(const ClientContextPtr& ctx, const std::string& url, const std::string& method,
                          HttpRequestProtocol* req);
};

}  // namespace trpc
