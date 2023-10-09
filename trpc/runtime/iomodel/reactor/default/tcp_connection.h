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

#include <deque>

#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/runtime/iomodel/reactor/common/io_message.h"
#include "trpc/runtime/iomodel/reactor/common/socket.h"
#include "trpc/runtime/iomodel/reactor/reactor.h"
#include "trpc/util/align.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {

/// @brief The connection for tcp
class TcpConnection final : public Connection {
 public:
  explicit TcpConnection(Reactor* reactor, const Socket& socket);

  ~TcpConnection() override;

  void StartHandshaking() override;

  void Established() override;

  bool DoConnect() override;

  void DoClose(bool destroy) override;

  int Send(IoMessage&& msg) override;

  /// @brief Close read event
  void DisableRead();

  void Throttle(bool set) {}

 protected:
  int HandleReadEvent() override;
  int HandleWriteEvent() override;
  void HandleCloseEvent() override;
  void EnableReadWrite();
  void DisableReadWrite();
  void UpdateWriteEvent();

 private:
  void Handshaking(bool is_read_event);
  void HandleClose(bool destroy);
  void MessageWriteDone(IoMessage& msg);
  int JudgeConnected();
  bool PreCheckOnWrite();
  int ReadIoData(NoncontiguousBuffer& buff);

 private:
  static constexpr int kSendDataMergeNum = 16;
  static constexpr uint32_t kMergeSendDataSize = 8192;
  static constexpr uint32_t kMaxIoMsgNum = 1024;

  Reactor* reactor_{nullptr};

  Socket socket_;

  // Whether read/write event is enabled
  bool enable_{false};

  // Handshake status
  IoHandler::HandshakeStatus handshake_status_{IoHandler::HandshakeStatus::kFailed};

  // Recv buffer
  struct alignas(hardware_destructive_interference_size) {
    BufferBuilder builder;
    NoncontiguousBuffer buffer;
  } read_buffer_;

  // Io message send queue
  std::deque<IoMessage> io_msgs_;

  // The data size of the message to be sent
  uint32_t send_data_size_{0};

  // Whether to need write data directly
  bool need_direct_write_{false};

  // When the connection is established,
  // notify the upper layer that the data cached in the queue can be written
  bool notify_cache_msg_in_queue_{false};
};

}  // namespace trpc
