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

#include "trpc/runtime/iomodel/reactor/fiber/writing_datagram_list.h"

#include <climits>
#include <random>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/runtime/iomodel/reactor/common/io_message.h"
#include "trpc/runtime/iomodel/reactor/common/socket.h"
#include "trpc/util/net_util.h"

namespace trpc::testing {

class MockConnHanlder : public ConnectionHandler {
 public:
  MockConnHanlder() {}

  ~MockConnHanlder() {}

  Connection* GetConnection() const override { return nullptr; }

  int CheckMessage(const ConnectionPtr&, NoncontiguousBuffer&, std::deque<std::any>&) override { return 0; }

  bool HandleMessage(const ConnectionPtr&, std::deque<std::any>&) override { return true; }
};

TEST(WritingDatagramList, Normal) {
  uint16_t recv_port = trpc::util::GenRandomAvailablePort();
  Socket recv_socket = Socket::CreateUdpSocket(false);
  Socket send_socket = Socket::CreateUdpSocket(false);
  NetworkAddress recv_addr("127.0.0.1", recv_port, NetworkAddress::IpType::kIpV4);
  recv_socket.Bind(recv_addr);

  WritingDatagramList wdl;
  IoMessage first_io_msg, second_io_msg;
  first_io_msg.buffer = CreateBufferSlow(std::string(1111, 'x').data());
  first_io_msg.seq_id = 123;

  second_io_msg.buffer = CreateBufferSlow(std::string(2222, 'x').data());
  second_io_msg.seq_id = 456;

  ASSERT_TRUE(wdl.Append(recv_addr, std::move(first_io_msg)));
  ASSERT_TRUE(wdl.Append(recv_addr, std::move(second_io_msg)));

  bool emptied;
  MockConnHanlder mock_handler;
  ssize_t send_size = wdl.FlushTo(send_socket, &mock_handler, &emptied);
  ASSERT_EQ(1111, send_size);
  ASSERT_FALSE(emptied);
  constexpr uint32_t kUdpBuffSize = 64 * 1024;
  char recv_buffer[kUdpBuffSize];
  NetworkAddress peer_addr;
  int recv_size = recv_socket.RecvFrom(recv_buffer, kUdpBuffSize, 0, &peer_addr);
  ASSERT_EQ(1111, recv_size);

  send_size = wdl.FlushTo(send_socket, &mock_handler, &emptied);
  ASSERT_EQ(2222, send_size);
  ASSERT_FALSE(emptied);
  recv_size = recv_socket.RecvFrom(recv_buffer, kUdpBuffSize, 0, &peer_addr);
  ASSERT_EQ(2222, recv_size);

  // highly fragmented
  size_t highly_fragmented_size = 0;
  IoMessage fragmented_io_msg;
  for (size_t i = 0; i < IOV_MAX + 1; i++) {
    fragmented_io_msg.buffer.Append(CreateBufferSlow("x", 1));
    highly_fragmented_size += 1;
  }
  ASSERT_TRUE(wdl.Append(recv_addr, std::move(fragmented_io_msg)));
  send_size = wdl.FlushTo(send_socket, &mock_handler, &emptied);
  ASSERT_EQ(highly_fragmented_size, send_size);
  ASSERT_FALSE(emptied);
  recv_size = recv_socket.RecvFrom(recv_buffer, kUdpBuffSize, 0, &peer_addr);
  ASSERT_EQ(highly_fragmented_size, recv_size);

  // the list is empty now
  send_size = wdl.FlushTo(send_socket, &mock_handler, &emptied);
  ASSERT_EQ(0, send_size);
  ASSERT_TRUE(emptied);
}

}  // namespace trpc::testing
