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

#include "trpc/admin/reload_config_handler.h"

#include <memory>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/server/server_context.h"

namespace trpc::testing {

class TestReloadConfigHandler : public ::testing::Test {
 public:
  void SetUp() {
    EXPECT_EQ(TrpcConfig::GetInstance()->Init("trpc/admin/test2.yaml"), 0);
    EXPECT_TRUE(log::Init());
  }

  void TearDown() { log::Destroy(); }
};

TEST_F(TestReloadConfigHandler, Test0) {
  http::HttpResponse reply;
  ServerContextPtr context;

  std::unique_ptr<AdminHandlerBase> h1 = std::make_unique<admin::ReloadConfigHandler>();
  h1->Handle("", context, std::make_shared<http::HttpRequest>(), &reply);
  EXPECT_EQ("{\"errorcode\":0,\"message\":\"reload config ok\"}", reply.GetContent());
}

}  // namespace trpc::testing
