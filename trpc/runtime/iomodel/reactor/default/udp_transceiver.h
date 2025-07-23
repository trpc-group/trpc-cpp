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

#include <deque>
#include <memory>

#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/runtime/iomodel/reactor/common/io_message.h"
#include "trpc/runtime/iomodel/reactor/common/socket.h"
#include "trpc/runtime/iomodel/reactor/reactor.h"
#include "trpc/util/align.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {

/// @brief The connection for udp. Actually, udp is connectionless. Inheriting from Connection here is for the
/// convenience of unified management by higher-level modules.
class UdpTransceiver final : public Connection {
 public:
  explicit UdpTransceiver(Reactor* reactor, const Socket& socket, bool is_client = false);

  ~UdpTransceiver() override;

  /// @brief Start listening for read/write events
  void EnableReadWrite();

  /// @brief Stop listening for read/write events
  void DisableReadWrite();

  void StartHandshaking() override;

  int Send(IoMessage&& msg) override;

  void DoClose(bool destroy) override;

 protected:
  int HandleReadEvent() override;

  int HandleWriteEvent() override;

  void HandleCloseEvent() override;

 private:
  // Call when a business request or response is successfully written to the network
  void MessageWriteDone(IoMessage& msg);

  // Serves as the default implement for reading IO data when the user does not have a custom IO read function
  int ReadIoData(NoncontiguousBuffer& buff);

  void HandleClose(bool destroy);

 private:
  // The maximum theoretical length of a udp packet. (The udp header occupies 8 bytes and the IP header occupies 20
  // bytes. So the maximum length of a udp packet is 2^16 - 1 - 8 - 20 = 65507 bytes.)
  static constexpr uint32_t kMaxUdpBodySize = 65507;

  Reactor* reactor_{nullptr};

  Socket socket_;

  // Whether read/write event is enabled
  bool enable_{false};

  // Recv buffer
  NoncontiguousBuffer read_buffer_;

  // Io message send queue
  std::deque<IoMessage> io_msgs_;

  // Size of udp buff
  static constexpr uint32_t kUdpBuffSize = 64 * 1024;
  // Store the data read from the RecvFrom call
  char recv_buffer_[kUdpBuffSize] = {0};
};

}  // namespace trpc
