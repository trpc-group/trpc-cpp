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

#include "trpc/stream/grpc/grpc_io_handler.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/runtime/iomodel/reactor/common/connection_handler.h"
#include "trpc/stream/testing/mock_connection.h"

namespace trpc::testing {

TEST(GrpcIoHandlerTest, HandshakeTest) {
  auto conn = std::make_unique<MockTcpConnection>();
  std::unique_ptr<MockConnectionHandler> conn_handler = std::make_unique<MockConnectionHandler>();
  EXPECT_CALL(*conn_handler, DoHandshake())
      .Times(2)
      .WillOnce(::testing::Return(0))
      .WillOnce(::testing::Return(-1));

  conn->SetConnectionHandler(std::move(conn_handler));
  GrpcIoHandler io_handler(conn.get());
  ASSERT_EQ(io_handler.Handshake(true), IoHandler::HandshakeStatus::kSucc);
  ASSERT_EQ(io_handler.Handshake(true), IoHandler::HandshakeStatus::kFailed);
}

}  // namespace trpc::testing
