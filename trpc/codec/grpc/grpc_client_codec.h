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

#include <any>
#include <deque>
#include <string>

#include "trpc/client/client_context.h"
#include "trpc/codec/client_codec.h"
#include "trpc/common/status.h"

namespace trpc {

/// @brief Client codec for gRPC Unary RPC.
class GrpcClientCodec : public ClientCodec {
 public:
  ~GrpcClientCodec() override = default;

  std::string Name() const override { return "grpc"; }

  /// @brief Decodes a complete grpc protocol message from binary byte stream (zero-copy).
  ///
  /// @param conn is the connection object where the protocol integrity check is performed on the current binary bytes.
  /// @param in is a buffer contains input binary bytes read from the socket.
  /// @param out is a list of successfully decoded protocol message.
  /// @return Returns a number greater than or equal to 0 on success, less than 0 on failure.
  int ZeroCopyCheck(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) override {
    return PacketChecker::PACKET_LESS;
  }

  /// @brief Encodes a grpc request protocol message object into a protocol message.
  ///
  /// @param ctx is client context for encoding.
  /// @param in  is grpc request protocol message object.
  /// @param out is a complete protocol message.
  /// @return Returns true on success, false otherwise.
  bool ZeroCopyEncode(const ClientContextPtr& ctx, const ProtocolPtr& in, NoncontiguousBuffer& out) override;


  /// @brief Decodes a grpc response protocol message object from a complete protocol message decoded from binary
  /// bytes (zero copy).
  ///
  /// @param ctx is client context for decoding.
  /// @param in is a complete protocol message decoded from binary bytes.
  ///           It is usually the output parameter of method "Check".
  /// @param out is grpc response protocol message object.
  /// @return Returns true on success, false otherwise.
  bool ZeroCopyDecode(const ClientContextPtr& ctx, std::any&& in, ProtocolPtr& out) override;

  /// @brief Fills the protocol object with the request message passed by user. If |body| is an IDL protocol message,
  /// you need to serialize it into binary data first, and then put it in the protocol message object.
  ///
  /// @param ctx is client context.
  /// @param in is grpc request protocol message object.
  /// @param body is the request message passed by user.
  /// @return Returns true on success, false otherwise.
  bool FillRequest(const ClientContextPtr& ctx, const ProtocolPtr& in, void* body) override;

  /// @brief Fills the response message with the protocol object. If |body| is an IDL protocol message,
  /// you need to deserialize binary data into a structure of response message first.
  ///
  /// @param ctx is client context.
  /// @param in is grpc response protocol message object.
  /// @param body is the response message expected by user.
  /// @return Returns true on success, false otherwise.
  bool FillResponse(const ClientContextPtr& ctx, const ProtocolPtr& in, void* body) override;

  /// @brief Creates a protocol object of grpc request (unary call).
  ProtocolPtr CreateRequestPtr() override;

  /// @brief Creates a protocol object of response (unary call).
  ProtocolPtr CreateResponsePtr() override;

  /// @brief Returns request id stored in response.
  uint32_t GetSequenceId(const ProtocolPtr& rsp) const override;

  /// @brief Extracts metadata of protocol message, e.g. frame-type of data, stream-id, frame-type of stream.
  bool Pick(const std::any& message, std::any& data) const override { return false; }

  /// @brief Reports whether connection multiplexing is supported.
  bool IsComplex() const override { return true; }

  /// @brief Reports whether streaming RPC is supported.
  bool HasStreamingRpc() const override { return false; }

  /// @brief Reports whether it is streaming protocol.
  bool IsStreamingProtocol() const override { return true; }

 private:
  bool Serialize(const ClientContextPtr& context, void* body, NoncontiguousBuffer* buffer);
  bool Deserialize(const ClientContextPtr& context, NoncontiguousBuffer* buffer, void* body);

  bool Compress(const ClientContextPtr& context, NoncontiguousBuffer* buffer);
  bool Decompress(const ClientContextPtr& context, NoncontiguousBuffer* buffer);

  std::string GetTimeoutText(uint32_t timeout);
};

}  // namespace trpc
