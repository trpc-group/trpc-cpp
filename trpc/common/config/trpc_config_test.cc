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

#include <iostream>

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"

namespace trpc::testing {

TEST(TrpcConfig, TestInit) {
  trpc::TrpcConfig* trpc_config = trpc::TrpcConfig::GetInstance();

  ASSERT_EQ(trpc_config->Init("./trpc/common/config/testing/fiber.yaml"), 0);

  const auto& global = trpc_config->GetGlobalConfig();
  ASSERT_EQ(global.threadmodel_config.fiber_model.size(), 1);

  const auto& server = trpc_config->GetServerConfig();
  ASSERT_EQ(server.app, "Test");

  const auto& client = trpc_config->GetClientConfig();
  ASSERT_EQ(client.filters.size(), 1);
}

}  // namespace trpc::testing
