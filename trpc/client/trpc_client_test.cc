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

#include "trpc/client/trpc_client.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/client/testing/service_proxy_testing.h"
#include "trpc/naming/trpc_naming_registry.h"
#include "trpc/runtime/runtime.h"

namespace trpc::testing {

class TrpcClientTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    EXPECT_EQ(TrpcConfig::GetInstance()->Init("trpc/client/testing/fiber_client.yaml"), 0);

    codec::Init();
    naming::Init();

    runtime::StartRuntime();
  }

  static void TearDownTestCase() {
    std::shared_ptr<TrpcClient> trpc_client = GetTrpcClient();
    if (trpc_client) {
      trpc_client->Stop();
    }

    naming::Stop();

    runtime::TerminateRuntime();

    if (trpc_client) {
      trpc_client->Destroy();
    }

    codec::Destroy();
    naming::Destroy();
  }

  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(TrpcClientTest, TestGetProxyOnlyByName) {
  std::shared_ptr<TrpcClient> trpc_client = GetTrpcClient();

  std::string name("");
  auto ptr1 = trpc_client->GetProxy<TestServiceProxy>(name);

  ASSERT_TRUE(ptr1 == nullptr);

  name = "http_client";

  // in framework config
  auto ptr2 = trpc_client->GetProxy<TestServiceProxy>(name);
  ASSERT_TRUE(ptr2 != nullptr);
  ASSERT_TRUE(ptr2->GetServiceProxyOption()->codec_name == "http");

  name = "trpc_client";

  // not in framework config
  auto ptr3 = trpc_client->GetProxy<TestServiceProxy>(name);
  ASSERT_TRUE(ptr3 != nullptr);
  ASSERT_TRUE(ptr3->GetServiceProxyOption()->codec_name == "trpc");
}

TEST_F(TrpcClientTest, TestGetProxyByCode) {
  std::shared_ptr<TrpcClient> trpc_client = GetTrpcClient();

  std::string name = "http_client2";

  // create by code , ignore framework config
  ServiceProxyOption option;
  option.name = name;
  option.codec_name = "trpc";
  option.max_conn_num = 10;
  option.is_conn_complex = true;
  option.selector_name = "direct";
  option.disable_servicerouter = true;

  auto ptr = trpc_client->GetProxy<TestServiceProxy>(name, option);
  ASSERT_TRUE(ptr != nullptr);
  ASSERT_TRUE(ptr->GetServiceProxyOption()->codec_name == "trpc");
}

TEST_F(TrpcClientTest, TestGetProxyByFunction) {
  std::shared_ptr<TrpcClient> trpc_client = GetTrpcClient();

  std::string name = "redis_client";

  // set some values of ServiceProxyOption by func
  auto func = [](ServiceProxyOption* option) {
    option->redis_conf.user_name = "test";
    option->redis_conf.password = "123456";
  };

  auto ptr = trpc_client->GetProxy<TestServiceProxy>(name, func);
  ASSERT_TRUE(ptr != nullptr);
  ASSERT_TRUE(ptr->GetServiceProxyOption()->redis_conf.user_name == "test");
  ASSERT_TRUE(ptr->GetServiceProxyOption()->redis_conf.password == "123456");
}

}  // namespace trpc::testing
