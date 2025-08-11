// trpc/codec/http_sse/http_sse_codec.cc
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

#include "trpc/codec/http_sse/http_sse_codec.h"

#include "trpc/codec/http/http_protocol.h"
#include "trpc/common/status.h"
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

  std::string serialized = http::sse::SseParser::SerializeEvent(event);
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
  std::string serialized = http::sse::SseParser::SerializeEvent(event);
  response.SetContent(serialized);
  response.SetMimeType("text/event-stream");
}

void HttpSseResponseProtocol::SetSseEvents(const std::vector<http::sse::SseEvent>& events) {
  std::string serialized = http::sse::SseParser::SerializeEvents(events);
  response.SetContent(serialized);
  response.SetMimeType("text/event-stream");
}

// HttpSseClientCodec implementation
bool HttpSseClientCodec::ZeroCopyDecode(const ClientContextPtr& ctx, std::any&& in, ProtocolPtr& out) {
  // First, use the parent HTTP codec to decode the basic HTTP response
  if (!HttpClientCodec::ZeroCopyDecode(ctx, std::move(in), out)) {
    return false;
  }

  // Cast to our SSE-specific protocol
  auto sse_protocol = std::dynamic_pointer_cast<HttpSseResponseProtocol>(out);
  if (!sse_protocol) {
    // If it's not our SSE protocol, create a new one and copy the data
    auto http_protocol = std::dynamic_pointer_cast<HttpResponseProtocol>(out);
    if (!http_protocol) {
      TRPC_LOG_ERROR("Failed to cast to HttpResponseProtocol");
      return false;
    }

    sse_protocol = std::make_shared<HttpSseResponseProtocol>();
    sse_protocol->response = http_protocol->response;
    out = sse_protocol;
  }

  return true;
}

bool HttpSseClientCodec::ZeroCopyEncode(const ClientContextPtr& ctx, const ProtocolPtr& in, NoncontiguousBuffer& out) {
  auto sse_protocol = std::dynamic_pointer_cast<HttpSseRequestProtocol>(in);
  if (!sse_protocol) {
    TRPC_LOG_ERROR("Failed to cast to HttpSseRequestProtocol");
    return false;
  }

  // Set SSE-specific headers
  if (sse_protocol->request) {
    SetSseRequestHeaders(sse_protocol->request.get());
  }

  // Use the parent HTTP codec to encode
  return HttpClientCodec::ZeroCopyEncode(ctx, in, out);
}

bool HttpSseClientCodec::FillRequest(const ClientContextPtr& ctx, const ProtocolPtr& in, void* body) {
  auto sse_protocol = std::dynamic_pointer_cast<HttpSseRequestProtocol>(in);
  if (!sse_protocol) {
    TRPC_LOG_ERROR("Failed to cast to HttpSseRequestProtocol");
    return false;
  }

  // If body is an SseEvent pointer, set it
  if (body) {
    auto* event = static_cast<http::sse::SseEvent*>(body);
    sse_protocol->SetSseEvent(*event);
  }

  // Set SSE-specific headers
  if (sse_protocol->request) {
    SetSseRequestHeaders(sse_protocol->request.get());
  }

  return true;
}

bool HttpSseClientCodec::FillResponse(const ClientContextPtr& ctx, const ProtocolPtr& in, void* body) {
  auto sse_protocol = std::dynamic_pointer_cast<HttpSseResponseProtocol>(in);
  if (!sse_protocol) {
    TRPC_LOG_ERROR("Failed to cast to HttpSseResponseProtocol");
    return false;
  }

  if (!body) {
    return true;
  }

  // Try to parse SSE events from the response
  auto event = sse_protocol->GetSseEvent();
  if (!event) {
    TRPC_LOG_ERROR("Failed to get SSE event from response");
    return false;
  }

  // Fill the body based on its type
  // Check if it's a single SseEvent pointer
  if (auto* single_event = static_cast<http::sse::SseEvent*>(body)) {
    *single_event = *event;
    return true;
  }

  // Check if it's a vector of SseEvent pointers
  if (auto* events_vector = static_cast<std::vector<http::sse::SseEvent>*>(body)) {
    events_vector->clear();
    events_vector->push_back(*event);
    return true;
  }

  TRPC_LOG_ERROR("Unknown body type for SSE response");
  return false;
}

ProtocolPtr HttpSseClientCodec::CreateRequestPtr() {
  return std::make_shared<HttpSseRequestProtocol>();
}

ProtocolPtr HttpSseClientCodec::CreateResponsePtr() {
  return std::make_shared<HttpSseResponseProtocol>();
}

void HttpSseClientCodec::SetSseRequestHeaders(http::Request* request) {
  if (!request) {
    return;
  }

  // Set Accept header for SSE
  request->SetHeader("Accept", "text/event-stream");
  
  // Set Cache-Control to prevent caching
  request->SetHeader("Cache-Control", "no-cache");
  
  // Set Connection to keep-alive for streaming
  request->SetHeader("Connection", "keep-alive");
}

void HttpSseClientCodec::SetSseResponseHeaders(http::Response* response) {
  if (!response) {
    return;
  }

  // Set Content-Type for SSE
  response->SetContentType("text/event-stream");
  
  // Set Cache-Control to prevent caching
  response->SetHeader("Cache-Control", "no-cache");
  
  // Set Connection to keep-alive for streaming
  response->SetHeader("Connection", "keep-alive");
}

// HttpSseServerCodec implementation
int HttpSseServerCodec::ZeroCopyCheck(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
  // Use the parent HTTP codec to check protocol integrity
  return HttpServerCodec::ZeroCopyCheck(conn, in, out);
}

bool HttpSseServerCodec::ZeroCopyDecode(const ServerContextPtr& ctx, std::any&& in, ProtocolPtr& out) {
  // First, use the parent HTTP codec to decode the basic HTTP request
  if (!HttpServerCodec::ZeroCopyDecode(ctx, std::move(in), out)) {
    return false;
  }

  // Cast to our SSE-specific protocol
  auto sse_protocol = std::dynamic_pointer_cast<HttpSseRequestProtocol>(out);
  if (!sse_protocol) {
    // If it's not our SSE protocol, create a new one and copy the data
    auto http_protocol = std::dynamic_pointer_cast<HttpRequestProtocol>(out);
    if (!http_protocol) {
      TRPC_LOG_ERROR("Failed to cast to HttpRequestProtocol");
      return false;
    }

    sse_protocol = std::make_shared<HttpSseRequestProtocol>();
    sse_protocol->request = http_protocol->request;
    sse_protocol->request_id = http_protocol->request_id;
    sse_protocol->from_http_service_proxy_ = http_protocol->from_http_service_proxy_;
    out = sse_protocol;
  }

  // Validate that this is a valid SSE request
  if (sse_protocol->request && !IsValidSseRequest(sse_protocol->request.get())) {
    TRPC_LOG_WARN("Invalid SSE request received");
    // Don't fail here, just log a warning
  }

  return true;
}

bool HttpSseServerCodec::ZeroCopyEncode(const ServerContextPtr& ctx, ProtocolPtr& in, NoncontiguousBuffer& out) {
  auto sse_protocol = std::dynamic_pointer_cast<HttpSseResponseProtocol>(in);
  if (!sse_protocol) {
    TRPC_LOG_ERROR("Failed to cast to HttpSseResponseProtocol");
    return false;
  }

  // Set SSE-specific headers
  SetSseResponseHeaders(&sse_protocol->response);

  // Use the parent HTTP codec to encode
  return HttpServerCodec::ZeroCopyEncode(ctx, in, out);
}

ProtocolPtr HttpSseServerCodec::CreateRequestObject() {
  return std::make_shared<HttpSseRequestProtocol>();
}

ProtocolPtr HttpSseServerCodec::CreateResponseObject() {
  return std::make_shared<HttpSseResponseProtocol>();
}

void HttpSseServerCodec::SetSseResponseHeaders(http::Response* response) {
  if (!response) {
    return;
  }

  // Set Content-Type for SSE
  response->SetContentType("text/event-stream");
  
  // Set Cache-Control to prevent caching
  response->SetHeader("Cache-Control", "no-cache");
  
  // Set Connection to keep-alive for streaming
  response->SetHeader("Connection", "keep-alive");
  
  // Set Access-Control-Allow-Origin for CORS (if needed)
  response->SetHeader("Access-Control-Allow-Origin", "*");
  
  // Set Access-Control-Allow-Headers for CORS
  response->SetHeader("Access-Control-Allow-Headers", "Cache-Control");
}

bool HttpSseServerCodec::IsValidSseRequest(const http::Request* request) const {
  if (!request) {
    return false;
  }

  // Check if Accept header includes text/event-stream
  std::string accept = request->GetHeader("Accept");
  if (accept.find("text/event-stream") == std::string::npos) {
    return false;
  }

  // SSE requests are typically GET requests
  if (request->GetMethod() != "GET") {
    return false;
  }

  return true;
}

}  // namespace trpc