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
#include <memory>
#include <string>

#include "trpc/codec/codec_helper.h"
#include "trpc/codec/protocol.h"
#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

class ServerContext;
using ServerContextPtr = RefPtr<ServerContext>;

/// @brief An interface (server-side) representing the ability to decode request message and encode response message.
class ServerCodec {
 public:
  virtual ~ServerCodec() = default;

  /// @brief Returns name of codec.
  /// Different protocol codec implementation need different names. When there multiple implementation codecs for the
  /// same protocol, they also need to have different names.
  virtual std::string Name() const = 0;

  /// @brief Decodes a complete protocol message from binary byte stream (zero-copy).
  /// @param conn Is the connection object where the protocol integrity check is performed on the current binary bytes.
  /// @param in Is a buffer contains input binary bytes read from the socket.
  /// @param out Is a list of successfully decoded protocol message, it may be the raw request or the decoded request
  ///             object, it depends on specific implementation.
  /// @return Returns a number greater than or equal to 0 on success, less than 0 on failure.
  virtual int ZeroCopyCheck(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
    return -1;
  }

  /// @brief Decodes a protocol message object from a complete protocol message decoded from binary bytes (zero copy).
  /// @param ctx Is server context for decoding.
  /// @param in Is a complete protocol message decoded from binary bytes.
  ///           It is usually the output parameter of method "Check".
  /// @param out Is protocol message object implements the "Protocol" interface.
  /// @return Returns true on success, false otherwise.
  virtual bool ZeroCopyDecode(const ServerContextPtr& ctx, std::any&& in, ProtocolPtr& out) { return false; }

  /// @brief Encodes a protocol message object into a protocol message.
  /// @param ctx Is server context for encoding.
  /// @param in  Is protocol message object implements the "Protocol" interface.
  /// @param out Is a complete protocol message in binary bytes which will be send to client over network.
  /// @return Returns true on success, false otherwise.
  virtual bool ZeroCopyEncode(const ServerContextPtr& ctx, ProtocolPtr& in, NoncontiguousBuffer& out) { return false; }

  /// @brief Creates a protocol object of request.
  virtual ProtocolPtr CreateRequestObject() = 0;

  /// @brief Creates a protocol object of response.
  virtual ProtocolPtr CreateResponseObject() = 0;

  /// @brief Extracts metadata of protocol message, e.g. frame-type of data, stream-id, frame-type of stream.
  virtual bool Pick(const std::any& message, std::any& data) const { return false; }

  /// @brief Reports whether streaming RPC is supported.
  virtual bool HasStreamingRpc() const { return false; }

  /// @brief Reports whether it is streaming protocol.
  /// @note Requirements: Both unary RPC and streaming RPC are transmitted by stream frame, e.g. gRPC, HTTP2
  virtual bool IsStreamingProtocol() const { return false; }

  /// @brief Return the return code of a specific protocol through a unified return code
  virtual int GetProtocolRetCode(codec::ServerRetCode ret_code) const {
    return codec::GetDefaultServerRetCode(ret_code);
  }
};

using ServerCodecPtr = std::shared_ptr<ServerCodec>;

}  // namespace trpc
