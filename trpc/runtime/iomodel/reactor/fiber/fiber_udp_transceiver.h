//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/io/native/datagram_transceiver.h.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <deque>
#include <memory>

#include "trpc/runtime/iomodel/reactor/common/network_address.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_connection.h"
#include "trpc/runtime/iomodel/reactor/fiber/writing_datagram_list.h"

namespace trpc {

/// @brief Fiber udp connection implementation. Actually, udp is connectionless. Inheriting from FiberConnection here is
/// for the convenience of unified management by higher-level modules.
class FiberUdpTransceiver final : public FiberConnection {
 public:
  explicit FiberUdpTransceiver(Reactor* reactor, const Socket& socket, bool is_client = false);

  ~FiberUdpTransceiver() override;

  /// @brief Start listening for read/write events
  void EnableReadWrite();

  int Send(IoMessage&& msg) override;

  void Stop() override;

  void Join() override;

 private:
  EventAction OnReadable() override;
  EventAction OnWritable() override;
  void OnError(int err) override;
  void OnCleanup(CleanupReason reason) override;

  enum class FlushStatus { kFlushed, kQuotaExceeded, kSystemBufferSaturated, kPartialWrite, kNothingWritten, kError };

  FiberUdpTransceiver::FlushStatus FlushWritingBuffer(std::size_t max_bytes);

  // Re-listen for write events
  void RestartWriteEvent();

  int SendWithDatagramList(IoMessage&& msg);

 private:
  Socket socket_;

  // The maximum theoretical length of a udp packet. (The udp header occupies 8 bytes and the IP header occupies 20
  // bytes. So the maximum length of a udp packet is 2^16 - 1 - 8 - 20 = 65507 bytes.)
  static constexpr uint32_t kMaxUdpBodySize = 65507;

  // Size of udp buff
  static constexpr uint32_t kUdpBuffSize = 64 * 1024;

  // The maximum number of udp packets that can be sent with each call to Send()
  std::size_t max_writes_percall_ = 64;

  // Recv buffer
  NoncontiguousBuffer read_buffer_;

  WritingDatagramList write_list_;

  // Listening address
  NetworkAddress udp_addr_;
};

}  // namespace trpc
