//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/io/detail/writing_buffer_list_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/iomodel/reactor/fiber/writing_buffer_list.h"

#include <memory>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/runtime/iomodel/reactor/common/connection_handler.h"
#include "trpc/runtime/iomodel/reactor/common/io_handler.h"

namespace trpc::testing {

class TestIoHandler : public IoHandler {
 public:
  TestIoHandler() {}

  ~TestIoHandler() override {}

  Connection* GetConnection() const override { return nullptr; }

  HandshakeStatus Handshake(bool is_read_event) override { return HandshakeStatus::kSucc; }

  int Read(void* buff, uint32_t length) override { return 0; }

  int Writev(const iovec* iov, int iovcnt) override {
    int size = 0;
    for (int i = 0; i < iovcnt; ++i) {
      size += iov[i].iov_len;
    }
    return size;
  }
};

class MockConnHanlder : public ConnectionHandler {
 public:
  MockConnHanlder() {}

  ~MockConnHanlder() {}

  Connection* GetConnection() const override { return nullptr; }

  int CheckMessage(const ConnectionPtr&, NoncontiguousBuffer&, std::deque<std::any>&) override { return 0; }

  bool HandleMessage(const ConnectionPtr&, std::deque<std::any>&) override { return true; }
};

class TestShortWriteIoHandler : public TestIoHandler {
 public:
  TestShortWriteIoHandler() {}

  ~TestShortWriteIoHandler() override {}

  int Writev(const iovec* iov, int iovcnt) override {
    int size = 0;
    for (int i = 0; i < iovcnt; ++i) {
      size += iov[i].iov_len;
    }
    return size - 10;
  }
};

void TestEmptied() {
  WritingBufferList buffer;
  IoMessage first_io_msg, second_io_msg;
  first_io_msg.seq_id = 1;
  second_io_msg.seq_id = 2;
  ASSERT_EQ(WritingBufferList::BufferAppendStatus::kAppendHead,
            buffer.Append(CreateBufferSlow("123"), std::move(first_io_msg), 0, 0));
  ASSERT_EQ(WritingBufferList::BufferAppendStatus::kAppendTail,
            buffer.Append(CreateBufferSlow("2234"), std::move(second_io_msg), 0, 0));

  std::unique_ptr<IoHandler> io_handler = std::make_unique<TestIoHandler>();
  bool emptied = false;
  bool short_write = true;
  MockConnHanlder mock_handler;

  ASSERT_EQ(buffer.FlushTo(io_handler.get(), &mock_handler, 7, 0, true, &emptied, &short_write), 7);
  ASSERT_TRUE(emptied);
  ASSERT_FALSE(short_write);
}

void TestPartialFlush() {
  WritingBufferList buffer;
  IoMessage first_io_msg, second_io_msg, third_io_msg;
  first_io_msg.seq_id = 1;
  second_io_msg.seq_id = 2;
  third_io_msg.seq_id = 3;
  ASSERT_EQ(WritingBufferList::BufferAppendStatus::kAppendHead,
            buffer.Append(CreateBufferSlow("123"), std::move(first_io_msg), 7, 0));
  ASSERT_EQ(WritingBufferList::BufferAppendStatus::kAppendTail,
            buffer.Append(CreateBufferSlow("2234"), std::move(second_io_msg), 7, 0));
  ASSERT_EQ(WritingBufferList::BufferAppendStatus::kTimeout,
            buffer.Append(CreateBufferSlow("2235"), std::move(third_io_msg), 7, 0));

  std::unique_ptr<IoHandler> io_handler = std::make_unique<TestIoHandler>();
  bool emptied = true;
  bool short_write = true;
  MockConnHanlder mock_handler;
  ASSERT_EQ(buffer.FlushTo(io_handler.get(), &mock_handler, 5, 0, false, &emptied, &short_write), 5);
  ASSERT_FALSE(emptied);
  ASSERT_FALSE(short_write);
}

void TestShortWrite() {
  constexpr auto kBufferSize = 64 * 1024 * 1024;

  WritingBufferList buffer;
  IoMessage io_msg;
  io_msg.seq_id = 1;
  buffer.Append(CreateBufferSlow(std::string(kBufferSize, 'x')), std::move(io_msg), 0, 0);
  std::unique_ptr<IoHandler> io_handler = std::make_unique<TestShortWriteIoHandler>();
  bool emptied = true;
  bool short_write = false;
  MockConnHanlder mock_handler;
  auto rc = buffer.FlushTo(io_handler.get(), &mock_handler, kBufferSize, 0, true, &emptied, &short_write);
  ASSERT_TRUE(rc > 0);
  ASSERT_LT(rc, kBufferSize);
  ASSERT_FALSE(emptied);
  ASSERT_TRUE(short_write);
}

TEST(WritingBufferList, All) {
  RunAsFiber([]() {
    TestEmptied();
    TestPartialFlush();
    TestShortWrite();
  });
}

}  // namespace trpc::testing
