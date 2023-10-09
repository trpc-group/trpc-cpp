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

#include "trpc/transport/common/io_handler_manager.h"

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(IoHandlerManagerTest, All) {
  ASSERT_TRUE(InitIoHandler());

  DestroyIoHandler();
}

}  // namespace trpc::testing
