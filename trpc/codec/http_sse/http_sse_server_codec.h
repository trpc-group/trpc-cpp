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

#include "trpc/codec/http/http_server_codec.h"
#include "trpc/codec/http_sse/http_sse_protocol.h"

namespace trpc {

/// @brief HTTP SSE codec (server-side) to decode request message and encode response message.
class HttpSseServerCodec : public HttpServerCodec {
 public:
  ~HttpSseServerCodec() override = default;

  /// @brief Returns name of HTTP SSE codec.
  std::string Name() const override { return kHttpSseCodecName; }

  /// @brief Decodes a complete protocol message from binary byte stream (zero-copy).
  ///
  /// @param conn is the connection object where the protocol integrity check is performed on the current binary bytes.
  /// @param in is a buffer contains input binary bytes read from the socket.
  /// @param out is a list of successfully decoded HTTP SSE request protocol message.
  /// @return Returns a number greater than or equal to 0 on success, less than 0 on failure.
  int ZeroCopyCheck(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override;

  /// @brief Decodes a HTTP SSE request protocol message object from a complete HTTP request.
  ///
  /// @param ctx is server context for decoding.
  /// @param in is a complete HTTP SSE request decoded from binary bytes.
  ///           It is usually the output parameter of method "Check".
  /// @param out is HTTP SSE request protocol message object.
  /// @return Returns true on success, false otherwise.
  bool ZeroCopyDecode(const ServerContextPtr& ctx, std::any&& in, ProtocolPtr& out) override;

  /// @brief Encodes a HTTP SSE response protocol message object into a HTTP response in binary bytes.
  ///
  /// @param ctx is server context for encoding.
  /// @param in  is protocol message object implements the "Protocol" interface.
  /// @param out is a complete protocol message in binary bytes which will be send to client over network.
  /// @return Returns true on success, false otherwise.
  bool ZeroCopyEncode(const ServerContextPtr& ctx, ProtocolPtr& in, NoncontiguousBuffer& out) override;

  /// @brief Creates a HTTP SSE request protocol object.
  ProtocolPtr CreateRequestObject() override;

  /// @brief Creates a HTTP SSE response protocol object.
  ProtocolPtr CreateResponseObject() override;

 private:
  /// @brief Set SSE-specific headers for the response.
  /// @param response The HTTP response to set headers for.
  void SetSseResponseHeaders(http::Response* response);

 public:
  /// @brief Check if the request is a valid SSE request.
  /// @param request The HTTP request to check.
  /// @return Returns true if it's a valid SSE request, false otherwise.
  bool IsValidSseRequest(const http::Request* request) const;
};

}  // namespace trpc
