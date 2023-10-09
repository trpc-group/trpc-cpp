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

#include "trpc/stream/grpc/util.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/runtime/iomodel/reactor/common/default_io_handler.h"
#include "trpc/stream/testing/mock_connection.h"

namespace trpc::testing {

using namespace trpc::stream::grpc;

class MockIoHandler : public DefaultIoHandler {
 public:
  explicit MockIoHandler(Connection* conn) : DefaultIoHandler(conn) {}
  MOCK_METHOD(int, Writev, (const iovec* iov, int iovcnt), (override));
};

TEST(GrpcUitlTest, SendHandshakingMessageOk) {
  auto conn = std::make_unique<MockTcpConnection>();
  MockIoHandler io_handler(conn.get());

  std::string data = "test_data";
  NoncontiguousBuffer buffer = CreateBufferSlow(data);

  EXPECT_CALL(io_handler, Writev).WillOnce(::testing::Return(data.size()));
  ASSERT_EQ(SendHandshakingMessage(&io_handler, std::move(buffer)), 0);
}

TEST(GrpcUitlTest, TSendHandshakingMessageOkTriggerRetry) {
  auto conn = std::make_unique<MockTcpConnection>();
  MockIoHandler io_handler(conn.get());

  std::string data = "test_data";
  NoncontiguousBuffer buffer = CreateBufferSlow(data);

  EXPECT_CALL(io_handler, Writev).WillOnce(::testing::Return(-1)).WillOnce(::testing::Return(data.size()));
  errno = EAGAIN;
  ASSERT_EQ(SendHandshakingMessage(&io_handler, std::move(buffer)), 0);
}

TEST(GrpcUitlTest, TestWriteBufferToConnUntilDoneNotOk) {
  auto conn = std::make_unique<MockTcpConnection>();
  MockIoHandler io_handler(conn.get());

  // Empty buffer.
  ASSERT_EQ(SendHandshakingMessage(&io_handler, NoncontiguousBuffer{}), 0);

  std::string data = "test_data";
  NoncontiguousBuffer buffer = CreateBufferSlow(data);
  // Failed to write.
  errno = EIO;
  EXPECT_CALL(io_handler, Writev).WillOnce(::testing::Return(-1));
  ASSERT_EQ(SendHandshakingMessage(&io_handler, std::move(buffer)), errno);
}

};  // namespace trpc::testing
