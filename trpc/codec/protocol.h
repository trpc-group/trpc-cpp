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

#include <memory>
#include <string>

#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {

using RpcCallType = uint8_t;

/// @brief Simplify the mapping name of pass-through information
using TransInfoMap = google::protobuf::Map<std::string, std::string>;

/// @brief The default function name of Non-RPC.
constexpr char* const kNonRpcName = const_cast<char*>("_non_rpc_");
/// @brief The default the function name of transparent forwarding RPC where the server just sends it to upstream.
constexpr char* const kTransparentRpcName = const_cast<char*>("_transparent_rpc_");

/// @brief Dyeing key
constexpr char TRPC_DYEING_KEY[] = "trpc-dyeing-key";

/// @brief A simple RPC where the client sends a request to the server and waits for a response to come back,
/// just like a normal function call.
constexpr RpcCallType kUnaryCall{0};

/// @brief A simple RPC where the client only sends a request to the server,
/// but does not wait for a response to come back.
constexpr RpcCallType kOnewayCall{1};

/// @brief A client-side streaming RPC where the client writes a sequence of messages and sends them to the server,
/// again using a provided stream. Once the client has finished writing messages, it waits for the server to read
/// them all and returns its response.
constexpr RpcCallType kClientStreamingCall{2};

/// @brief A server-side streaming RPC where the client sends a request to the server and gets a stream to read
/// a sequence of message back. The client reads from the returned stream until there no more messages.
constexpr RpcCallType kServerStreamingCall{3};

/// @brief A bidirectional streaming RPC where both sides send a sequence of messages using a read-write stream.
/// The two streams operate independently, so clients and servers can read and write in whatever order they like.
constexpr RpcCallType kBidirectionalStreamingCall{4};

// for compatible
enum EncodeType {
  PB = TrpcContentEncodeType::TRPC_PROTO_ENCODE,
  JCE = TrpcContentEncodeType::TRPC_JCE_ENCODE,
  JSON = TrpcContentEncodeType::TRPC_JSON_ENCODE,
  FLATBUFFER = TrpcContentEncodeType::TRPC_FLATBUFFER_ENCODE,
  NOOP = TrpcContentEncodeType::TRPC_NOOP_ENCODE,
  THRIFT = TrpcContentEncodeType::TRPC_THRIFT_ENCODE,
  MAX_TYPE = 255,
};

/// @brief An interface representing the ability to decode and encode a protocol message.
/// It also has a list of Setter/Getter methods for metadata/header/body of protocol message object.
class Protocol {
 public:
  virtual ~Protocol() = default;

  /// @brief Decodes or encodes a protocol message (zero copy).
  virtual bool ZeroCopyDecode(NoncontiguousBuffer& buff) = 0;
  virtual bool ZeroCopyEncode(NoncontiguousBuffer& buff) = 0;

  /// @brief  Reports whether waits for the full request message content to arrive (For the RPC-Over-HTTP scene).
  virtual bool WaitForFullRequest() { return true; }

  /// @brief Set/Get body payload of protocol message.
  /// @note For "GetNonContiguousProtocolBody" function, the value of body will be moved, cannot be called repeatedly.
  virtual void SetNonContiguousProtocolBody(NoncontiguousBuffer&& buff) {}
  virtual NoncontiguousBuffer GetNonContiguousProtocolBody() { return NoncontiguousBuffer{}; }

  /// @brief Set/Get attachment payload of protocol message.
  /// @note For "GetProtocolAttachment" function, the value of body will be moved, cannot be called repeatedly.
  virtual void SetProtocolAttachment(NoncontiguousBuffer&& buff) {}
  virtual NoncontiguousBuffer GetProtocolAttachment() { return NoncontiguousBuffer{}; }

  /// @brief Get/Set the unique id of request/response protocol, if request/response protocol has
  virtual bool GetRequestId(uint32_t& req_id) const { return false; }
  virtual bool SetRequestId(uint32_t req_id) { return false; }

  /// @brief Get/Set the timeout of request/response protocol, depends on the implementation of the specific protocol.
  virtual void SetTimeout(uint32_t timeout) { timeout_ = timeout; }
  virtual uint32_t GetTimeout() const { return timeout_; }

  /// @brief Set/Get name of caller, depends on the implementation of the specific protocol.
  virtual void SetCallerName(std::string caller_name) { caller_ = std::move(caller_name); }
  virtual const std::string& GetCallerName() const { return caller_; }

  /// @brief Set/Get name of callee, depends on the implementation of the specific protocol.
  virtual void SetCalleeName(std::string callee_name) { callee_ = std::move(callee_name); }
  virtual const std::string& GetCalleeName() const { return callee_; }

  /// @brief  Set/Get function name of RPC, depends on the implementation of the specific protocol.
  virtual void SetFuncName(std::string func_name) { func_ = std::move(func_name); }
  virtual const std::string& GetFuncName() const { return func_; }

  /// @brief Set key-value pair, depends on the implementation of the specific protocol.
  virtual void SetKVInfo(std::string key, std::string value) { trans_info_[key] = value; }

  /// @brief Get key-value information from the protocol, and the specific protocol
  ///        implementation needs to be considered for implementation.
  /// @return const TransInfoMap&
  virtual const TransInfoMap& GetKVInfos() const { return trans_info_; }

  /// @brief Returns mutable key-value pairs, depends on the implementation of the specific protocol.
  virtual TransInfoMap* GetMutableKVInfos() { return &trans_info_; }

  /// @brief Get size of message
  virtual uint32_t GetMessageSize() const { return 0; }

  /// @brief Get the size of request/response protocol
  virtual size_t ByteSizeLong() const noexcept { return 0; }

  /// @brief Sets the reusable status of the connection in the protocol; specific protocol implementations need to be
  /// taken into consideration in order to implement this.
  virtual void SetConnectionReusable(bool connection_reusable) {}

  /// @brief Gets the reusable status of the connection in the protocol; specific protocol implementations need to be
  /// taken into consideration in order to implement this.
  virtual bool IsConnectionReusable() const { return true; }

 private:
  uint32_t timeout_{UINT32_MAX};
  std::string caller_;
  std::string callee_;
  std::string func_;
  TransInfoMap trans_info_;
};

using ProtocolPtr = std::shared_ptr<Protocol>;

}  // namespace trpc
