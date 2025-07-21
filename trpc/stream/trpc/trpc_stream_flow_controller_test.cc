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

#include "trpc/stream/trpc/trpc_stream_flow_controller.h"

#include "gtest/gtest.h"

namespace trpc::testing {

// For testing purposes only.
using namespace trpc::stream;

TEST(TrpcStreamSendControllerTest, TestWindowGetAndUpdate) {
  auto send_controller = MakeRefCounted<TrpcStreamSendController>(100);
  ASSERT_TRUE(send_controller->DecreaseWindow(105));
  ASSERT_FALSE(send_controller->DecreaseWindow(10));
  ASSERT_FALSE(send_controller->IncreaseWindow(5));
  ASSERT_TRUE(send_controller->IncreaseWindow(10));
  ASSERT_TRUE(send_controller->DecreaseWindow(100));
}

TEST(TrpcStreamRecvController, TestOnRecv) {
  auto recv_controller = MakeRefCounted<TrpcStreamRecvController>(100);
  ASSERT_EQ(recv_controller->UpdateConsumeBytes(10), 0);
  ASSERT_EQ(recv_controller->UpdateConsumeBytes(30), 40);
  ASSERT_EQ(recv_controller->UpdateConsumeBytes(0), 0);
}

}  // namespace trpc::testing
