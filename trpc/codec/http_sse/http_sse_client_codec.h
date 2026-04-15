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

#include "trpc/codec/http/http_client_codec.h"
#include "trpc/codec/http_sse/http_sse_protocol.h"

namespace trpc {

/// @brief HTTP SSE codec (client-side) to encode request message and decode response message.
class HttpSseClientCodec : public HttpClientCodec {
 public:
  ~HttpSseClientCodec() override = default;

  /// @brief Returns name of HTTP SSE codec.
  std::string Name() const override { return kHttpSseCodecName; }

  /// @brief Decodes a complete protocol message from binary byte stream (zero-copy).
  ///
  /// @param conn is the connection object where the protocol integrity check is performed on the current binary bytes.
  /// @param in is a buffer contains input binary bytes read from the socket.
  /// @param out is a list of successfully decoded HTTP SSE response protocol message.
  /// @return Returns a number greater than or equal to 0 on success, less than 0 on failure.
  int ZeroCopyCheck(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override;

  /// @brief Decodes a protocol message object from a complete protocol message decoded from binary bytes (zero copy).
  ///
  /// @param ctx is client context for decoding.
  /// @param in is a complete HTTP SSE response decoded from binary bytes.
  ///           It is usually the output parameter of method "Check".
  /// @param out is HTTP SSE response protocol message object.
  /// @return Returns true on success, false otherwise.
  bool ZeroCopyDecode(const ClientContextPtr& ctx, std::any&& in, ProtocolPtr& out) override;

  /// @brief Encodes a protocol message object into a protocol message.
  ///
  /// @param ctx is client context for encoding.
  /// @param in  is HTTP SSE request protocol message object.
  /// @param out is a complete protocol message in binary bytes which will be send to server over network.
  /// @return Returns true on success, false otherwise.
  bool ZeroCopyEncode(const ClientContextPtr& ctx, const ProtocolPtr& in, NoncontiguousBuffer& out) override;

  /// @brief Fills the protocol object with the request message passed by user.
  ///
  /// @param ctx is client context.
  /// @param in is HTTP SSE request protocol message object.
  /// @param body is the request message passed by user (can be SseEvent*).
  /// @return Returns true on success, false otherwise.
  bool FillRequest(const ClientContextPtr& ctx, const ProtocolPtr& in, void* body) override;

  /// @brief Fills the response message with the protocol object.
  ///
  /// @param ctx is client context.
  /// @param in is HTTP SSE response protocol message object.
  /// @param body is the response message expected by user (can be SseEvent* or std::vector<SseEvent>*).
  /// @return Returns true on success, false otherwise.
  bool FillResponse(const ClientContextPtr& ctx, const ProtocolPtr& in, void* body) override;

  /// @brief Creates a HTTP SSE request protocol object.
  ProtocolPtr CreateRequestPtr() override;

  /// @brief Creates a HTTP SSE response protocol object.
  ProtocolPtr CreateResponsePtr() override;

 private:
  /// @brief Set SSE-specific headers for the request.
  /// @param request The HTTP request to set headers for.
  void SetSseRequestHeaders(http::Request* request);

  /// @brief Set SSE-specific headers for the response.
  /// @param response The HTTP response to set headers for.
  void SetSseResponseHeaders(http::Response* response);
};

}  // namespace trpc
