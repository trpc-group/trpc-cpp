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

#include "trpc/codec/http_sse/http_sse_protocol.h"

#include "trpc/log/trpc_log.h"
#include "trpc/util/http/sse/sse_parser.h"

namespace trpc {

// HttpSseRequestProtocol implementation
std::optional<http::sse::SseEvent> HttpSseRequestProtocol::GetSseEvent() const {
  if (!request) {
    return std::nullopt;
  }

  std::string body = request->GetContent();
  if (body.empty()) {
    return std::nullopt;
  }

  try {
    return http::sse::SseParser::ParseEvent(body);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Failed to parse SSE event from request body: " << e.what());
    return std::nullopt;
  }
}

void HttpSseRequestProtocol::SetSseEvent(const http::sse::SseEvent& event) {
  if (!request) {
    request = std::make_shared<http::Request>();
  }

  std::string serialized = event.ToString();
  request->SetContent(serialized);
  request->SetHeader("Content-Type", "text/event-stream");
}

// HttpSseResponseProtocol implementation
std::optional<http::sse::SseEvent> HttpSseResponseProtocol::GetSseEvent() const {
  std::string body = response.GetContent();
  if (body.empty()) {
    return std::nullopt;
  }

  try {
    return http::sse::SseParser::ParseEvent(body);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Failed to parse SSE event from response body: " << e.what());
    return std::nullopt;
  }
}

void HttpSseResponseProtocol::SetSseEvent(const http::sse::SseEvent& event) {
  std::string serialized = event.ToString();
  response.SetContent(serialized);
  response.SetMimeType("text/event-stream");
}

void HttpSseResponseProtocol::SetSseEvents(const std::vector<http::sse::SseEvent>& events) {
  std::string serialized;
  for (const auto& event : events) {
    serialized += event.ToString();
  }
  response.SetContent(serialized);
  response.SetMimeType("text/event-stream");
}

}  // namespace trpc
