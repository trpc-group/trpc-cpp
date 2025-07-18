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

#include "trpc/transport/common/connection_handler_manager.h"

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(ConnectionHandlerManagerTest, All) {
  ASSERT_TRUE(InitConnectionHandler());

  DestroyConnectionHandler();
}

}  // namespace trpc::testing
