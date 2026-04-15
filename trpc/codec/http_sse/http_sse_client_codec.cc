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

#include "trpc/codec/http_sse/http_sse_client_codec.h"

#include "trpc/codec/http_sse/http_sse_proto_checker.h"
#include "trpc/common/status.h"
#include "trpc/log/trpc_log.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {



// HttpSseClientCodec implementation
int HttpSseClientCodec::ZeroCopyCheck(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
  return HttpSseZeroCopyCheckResponse(conn, in, out);
}

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

  // Set default HTTP method for SSE requests
  if (sse_protocol->request && sse_protocol->request->GetMethod().empty()) {
    sse_protocol->request->SetMethodType(http::OperationType::GET);
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
  std::string content = sse_protocol->response.GetContent();
  if (content.empty()) {
    TRPC_LOG_ERROR("Empty SSE response content");
    return false;
  }

  // Try to parse multiple events first
  try {
    auto events = http::sse::SseParser::ParseEvents(content);
    if (!events.empty()) {
      // Check if it's a vector of SseEvent pointers
      if (auto* events_vector = static_cast<std::vector<http::sse::SseEvent>*>(body)) {
        *events_vector = events;
        return true;
      }
      
      // Check if it's a single SseEvent pointer (use the first event)
      if (auto* single_event = static_cast<http::sse::SseEvent*>(body)) {
        *single_event = events[0];
        return true;
      }
    }
  } catch (const std::exception& e) {
    TRPC_LOG_WARN("Failed to parse multiple SSE events: " << e.what());
  }

  // Fallback to single event parsing
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
  response->SetMimeType("text/event-stream");
  
  // Set Cache-Control to prevent caching
  response->SetHeader("Cache-Control", "no-cache");
  
  // Set Connection to keep-alive for streaming
  response->SetHeader("Connection", "keep-alive");
}

}  // namespace trpc
