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

#include <any>
#include <deque>
#include <functional>
#include <memory>

#include "trpc/runtime/iomodel/reactor/common/accept_connection_info.h"
#include "trpc/runtime/iomodel/reactor/common/io_message.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {

/// @brief The request packet was not received completely when parse protocol,
///        continue to wait for data to be received
constexpr int kPacketLess = 0;
/// @brief At least one complete request packet has been received when parse protocol
constexpr int kPacketFull = 1;
/// @brief Parse protocol error, connection will be closed
constexpr int kPacketError = -1;

/// @brief Has deprecated
enum PacketChecker : int8_t {
  PACKET_LESS = 0,
  PACKET_FULL = 1,
  PACKET_ERR = -1,
};

/// @brief Connection type
enum ConnectionType : uint8_t {
  kTcpLong = 0,
  kTcpShort = 1,
  kUdp = 2,
  kOther = 3,
};

/// @brief Connection state
enum ConnectionState : uint8_t {
  kUnconnected = 0,
  kConnecting = 1,
  kConnected = 2,
};

/// @brief Connection error code
enum ConnectionErrorCode : uint8_t {
  // The error code of connection establishment timeout
  kConnectingTimeout = 0,
  // The error code of the network exception after the connection is established
  kNetworkException = 1,
};

class Connection;
using ConnectionPtr = trpc::RefPtr<Connection>;

/// @brief The function that check whether a complete message was received
using ProtocolCheckerFunction = std::function<int(const ConnectionPtr&, NoncontiguousBuffer&, std::deque<std::any>&)>;

/// @brief The function that process the complete message received
using MessageHandleFunction = std::function<bool(const ConnectionPtr&, std::deque<std::any>&)>;

/// @brief The function after send messages success
using MessageWriteDoneFunction = std::function<void(const IoMessage&)>;

/// @brief The function after the connection is successfully established
using ConnectionEstablishFunction = std::function<void(const Connection*)>;

/// @brief The function after the connection is closed
using ConnectionCloseFunction = std::function<void(const Connection*)>;

/// @brief The function for cleaning up connection-related resources
///        after connection closed
using ConnectionCleanFunction = std::function<void(Connection*)>;

/// @brief The function after the connection is established timeout
using ConnectionTimeoutFunction = std::function<void(const Connection*)>;

/// @brief The function that activates the connection-related info
///        when the connection has read and write data
///        eg: for idle connection close
using ConnectionTimeUpdateFunction = std::function<void(const Connection*)>;

/// @brief Get merge request count function
using GetMergeRequestCountFunction = std::function<uint32_t()>;

/// @brief The function that notify send message
///        after the connection is successfully established
using NotifySendMsgFunction = std::function<void(const Connection*)>;

/// @brief The function that notifies the id
///        after the connection pipeline sends a message successfully
using PipelineSendNotifyFunction = std::function<void(const IoMessage&)>;

/// @brief Base class for connection hander
///        eg: protocol handshake/analysis/resource cleanup, etc.
class ConnectionHandler {
 public:
  virtual ~ConnectionHandler() = default;

  /// @brief Get the connection which this connection handler belongs to
  virtual Connection* GetConnection() const = 0;

  /// @brief Handles the handshake of the application layer protocol
  /// @return 0: success, others: failed
  virtual int DoHandshake() { return 0; }

  /// @brief Check whether a complete message was received
  virtual int CheckMessage(const ConnectionPtr&, NoncontiguousBuffer&, std::deque<std::any>&) = 0;

  /// @brief Process the complete message received
  virtual bool HandleMessage(const ConnectionPtr&, std::deque<std::any>&) = 0;

  /// @brief Callback interface for cleaning up connection-related resources
  ///        after connection closed
  virtual void CleanResource() {}

  /// @brief Callback interface after the connection is successfully established
  virtual void ConnectionEstablished() {}

  /// @brief Callback interface after the connection is established timeout
  virtual void ConnectionTimeout() {}

  /// @brief Callback interface that notify send message
  ///        after the connection is successfully established
  virtual void NotifySendMessage() {}

  /// @brief Callback interface that activates the connection-related info
  ///        when the connection has read and write data
  ///        eg: for idle connection close
  virtual void UpdateConnection() {}

  /// @brief Callback interface after the connection is closed
  virtual void ConnectionClosed() {}

  /// @brief Callback interface before send messages
  virtual void PreSendMessage(const IoMessage&) {}

  /// @brief Callback interface after send messages success
  virtual void MessageWriteDone(const IoMessage&) {}

  /// @brief Callback interface after send messages failed
  virtual void NotifyMessageFailed(const IoMessage&, ConnectionErrorCode) {}

  /// @brief For stream, to initilize StreamHandler
  virtual void Init() {}

  /// @brief Perform stateful encoding of messages before sending them for a partial stream (such as with gRPC
  ///        streaming).
  virtual bool EncodeStreamMessage(IoMessage*) { return true; }

  /// @brief For stream, stop the streams on the connection when the connection is closed
  virtual void Stop() {}

  /// @brief For stream, wait the streams finished on the connectionwhen the connection is closed
  virtual void Join() {}

  /// @brief get the number of requests that merge together to send
  ///        For redis pipeline use
  virtual uint32_t GetMergeRequestCount() { return 1; }

  /// @brief Set/Get some state information saved in the protocol parsing process
  ///        eg: handle http HEAD request
  virtual void SetCurrentContextExt(uint32_t context_ext) {}
  virtual uint32_t GetCurrentContextExt() { return 0; }
};

}  // namespace trpc
