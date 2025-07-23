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

#include "trpc/admin/commands_handler.h"

#include "gtest/gtest.h"

#include <memory>
#include <utility>

namespace trpc::testing {

constexpr char kCommandsDescription[] = "[GET /cmds]      list supported cmds";

TEST(TestCommandsHandler, Test) {
  std::unique_ptr<AdminHandlerBase> h1 = std::make_unique<admin::CommandsHandler>(nullptr);
  EXPECT_EQ(kCommandsDescription, h1->Description());
}

}  // namespace trpc::testing
