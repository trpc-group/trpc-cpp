//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include "trpc/codec/http/http_protocol.h"
#include "trpc/codec/server_codec.h"
#include "trpc/server/server_context.h"

namespace trpc {

/// @brief HTTP codec (server-side) to decode request message and encode response message.
class HttpServerCodec : public ServerCodec {
 public:
  ~HttpServerCodec() override = default;

  /// @brief Returns name of HTTP codec.
  std::string Name() const override { return kHttpCodecName; }

  /// @brief Decodes a complete protocol message from binary byte stream (zero-copy).
  ///
  /// @param conn is the connection object where the protocol integrity check is performed on the current binary bytes.
  /// @param in is a buffer contains input binary bytes read from the socket.
  /// @param out is a list of successfully decoded HTTP request protocol message.
  /// @return Returns a number greater than or equal to 0 on success, less than 0 on failure.
  int ZeroCopyCheck(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override;

  /// @brief Decodes a HTTP request protocol message object from a complete HTTP request.
  ///
  /// @param ctx is server context for decoding.
  /// @param in is a complete HTTP request decoded from binary bytes.
  ///           It is usually the output parameter of method "Check".
  /// @param out is HTTP request protocol message object.
  /// @return Returns true on success, false otherwise.
  bool ZeroCopyDecode(const ServerContextPtr& ctx, std::any&& in, ProtocolPtr& out) override;

  /// @brief Encodes a HTTP response protocol message object into a HTTP response in binary bytes.
  ///
  /// @param ctx is server context for encoding.
  /// @param in  is protocol message object implements the "Protocol" interface.
  /// @param out is a complete protocol message in binary bytes which will be send to client over network.
  /// @return Returns true on success, false otherwise.
  bool ZeroCopyEncode(const ServerContextPtr& ctx, ProtocolPtr& in, NoncontiguousBuffer& out) override;

  /// @brief Creates a HTTP request protocol object.
  ProtocolPtr CreateRequestObject() override;

  /// @brief Creates a HTTP response protocol object.
  ProtocolPtr CreateResponseObject() override;
};

/// @brief TrpcOverHttp codec (server-side) to decode request message and encode response message.
class TrpcOverHttpServerCodec : public HttpServerCodec {
 public:
  std::string Name() const override { return kTrpcOverHttpCodecName; }

  bool ZeroCopyDecode(const ServerContextPtr& ctx, std::any&& in, ProtocolPtr& out) override;

  bool ZeroCopyEncode(const ServerContextPtr& ctx, ProtocolPtr& in, NoncontiguousBuffer& out) override;
};

}  // namespace trpc
