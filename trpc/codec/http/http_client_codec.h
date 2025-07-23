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
#include "trpc/codec/client_codec.h"
#include "trpc/codec/http/http_protocol.h"

namespace trpc {

/// @brief HTTP codec (client-side) to encode request message and decode response message.
class HttpClientCodec : public ClientCodec {
 public:
  ~HttpClientCodec() override = default;

  /// @brief Returns name of HTTP codec.
  std::string Name() const override { return kHttpCodecName; }

  /// @brief Decodes a complete HTTP protocol message from binary byte stream (zero-copy).
  ///
  /// @param conn is the connection object where the protocol integrity check is performed on the current binary bytes.
  /// @param in is a buffer contains input binary bytes read from the socket.
  /// @param out is a list of successfully decoded HTTP response.
  /// @return Returns a number greater than or equal to 0 on success, less than 0 on failure.
  int ZeroCopyCheck(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override;

  /// @brief Decodes a protocol message object from a complete protocol message decoded from binary bytes (zero copy).
  ///
  /// @param ctx is client context for decoding.
  /// @param in is a complete HTTP response decoded from binary bytes.
  ///           It is usually the output parameter of method "Check".
  /// @param out is HTTP response protocol message object.
  /// @return Returns true on success, false otherwise.
  bool ZeroCopyDecode(const ClientContextPtr& ctx, std::any&& in, ProtocolPtr& out) override;

  /// @brief Encodes a protocol message object into a protocol message.
  ///
  /// @param ctx is client context for encoding.
  /// @param in  is HTTP request protocol message object.
  /// @param out is a complete protocol message in binary bytes which will be send to server over network.
  /// @return Returns true on success, false otherwise.
  bool ZeroCopyEncode(const ClientContextPtr& ctx, const ProtocolPtr& in, NoncontiguousBuffer& out) override;

  /// @brief Fills the protocol object with the request message passed by user. If |body| is an IDL protocol message,
  /// you need to serialize it into binary data first, and then put it in the protocol message object.
  ///
  /// @param ctx is client context.
  /// @param in is HTTP request protocol message object.
  /// @param body is the request message passed by user.
  /// @return Returns true on success, false otherwise.
  bool FillRequest(const ClientContextPtr& ctx, const ProtocolPtr& in, void* body) override;

  /// @brief Fills the response message with the protocol object. If |body| is an IDL protocol message,
  /// you need to deserialize binary data into a structure of response message first.
  ///
  /// @param ctx is client context.
  /// @param in is HTTP response protocol message object.
  /// @param body is the response message expected by user.
  /// @return Returns true on success, false otherwise.
  bool FillResponse(const ClientContextPtr& ctx, const ProtocolPtr& in, void* body) override;

  /// @brief Creates a HTTP request protocol object.
  ProtocolPtr CreateRequestPtr() override;

  /// @brief Creates a HTTP response protocol object.
  ProtocolPtr CreateResponsePtr() override;
};

/// @brief TrpcOverHttp codec (client-side) to encode request message and decode response message.
class TrpcOverHttpClientCodec : public HttpClientCodec {
 public:
  std::string Name() const override { return kTrpcOverHttpCodecName; }
};

}  // namespace trpc
