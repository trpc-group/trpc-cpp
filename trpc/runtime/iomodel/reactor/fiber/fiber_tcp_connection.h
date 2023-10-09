//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/io/native/stream_connection.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <list>
#include <memory>

#include "trpc/runtime/iomodel/reactor/common/io_handler.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_connection.h"
#include "trpc/runtime/iomodel/reactor/fiber/writing_buffer_list.h"

namespace trpc {

/// @brief Fiber tcp connection implementation
class FiberTcpConnection final : public FiberConnection {
 public:
  explicit FiberTcpConnection(Reactor* reactor, const Socket& socket);

  ~FiberTcpConnection() override;

  void Established() override;

  bool DoConnect() override;

  void StartHandshaking() override;

  int Send(IoMessage&& msg) override;

  void DoClose(bool destroy) override;

  void Stop() override;

  void Join() override;

 private:
  enum class ReadStatus { kDrained, kPartialRead, kRemoteClose, kError };

  enum class FlushStatus {
    kFlushed,
    kQuotaExceeded,
    kSystemBufferSaturated,
    kPartialWrite,
    kNothingWritten,
    kError
  };

  EventAction OnReadable() override;
  EventAction OnWritable() override;
  void OnError(int err) override;
  void OnCleanup(CleanupReason reason) override;
  IoHandler::HandshakeStatus DoHandshake(bool from_on_readable);
  FiberTcpConnection::FlushStatus FlushWritingBuffer(std::size_t max_bytes);
  FiberTcpConnection::ReadStatus ReadData();
  FiberConnection::EventAction ConsumeReadData();

 private:
  struct HandshakingState {
    std::atomic<bool> done{false};
    std::mutex lock;
    bool need_restart_read{true};
    bool pending_restart_writes{false};
  };

  Socket socket_;

  // Describes state of handshaking.
  HandshakingState handshaking_state_;

  // Recv buffer
  struct alignas(hardware_destructive_interference_size) {
    BufferBuilder builder;
    NoncontiguousBuffer buffer;
  } read_buffer_;

  // Send buffer list
  alignas(hardware_destructive_interference_size) WritingBufferList writing_buffers_;
};

}  // namespace trpc
