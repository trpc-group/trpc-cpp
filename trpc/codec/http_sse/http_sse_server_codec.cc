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

#include "trpc/codec/http_sse/http_sse_server_codec.h"

#include "trpc/codec/http_sse/http_sse_proto_checker.h"
#include "trpc/common/status.h"
#include "trpc/log/trpc_log.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {



// HttpSseServerCodec implementation
int HttpSseServerCodec::ZeroCopyCheck(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
  // Use the SSE-specific protocol checker
  return HttpSseZeroCopyCheckRequest(conn, in, out);
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
  try {
    auto sse_protocol = std::dynamic_pointer_cast<HttpSseResponseProtocol>(in);
    if (!sse_protocol) {
      TRPC_LOG_ERROR("Failed to cast to HttpSseResponseProtocol");
      return false;
    }

    // Set SSE-specific headers
    SetSseResponseHeaders(&sse_protocol->response);

    // Serialize the HTTP response to binary format
    NoncontiguousBuffer buffer;
    sse_protocol->response.SerializeToString(buffer);
    out = std::move(buffer);

    return true;
  } catch (const std::exception& ex) {
    TRPC_LOG_ERROR("HTTP SSE encode throw exception: " << ex.what());
    return false;
  }
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
  response->SetMimeType("text/event-stream");
  
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

  // Check for other SSE-specific headers
  std::string cache_control = request->GetHeader("Cache-Control");
  if (cache_control.find("no-cache") == std::string::npos) {
    return false;
  }

  return true;
}

}  // namespace trpc
