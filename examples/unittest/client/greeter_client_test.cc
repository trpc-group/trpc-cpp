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

#include "examples/unittest/client/greeter_client.h"

#include <memory>

#include "gflags/gflags.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/trpc_client.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/future/future_utility.h"

#include "examples/unittest/helloworld.trpc.pb.h"
#include "examples/unittest/helloworld.trpc.pb.mock.h"

namespace test {
namespace unittest {
namespace testing {

using MockGreeterServiceProxyPtr = std::shared_ptr<::trpc::test::unittest::MockGreeterServiceProxy>;

class GreeterClientTestFixture : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    mock_service_proxy_ =
        ::trpc::GetTrpcClient()->GetProxy<::trpc::test::unittest::MockGreeterServiceProxy>("mock_proxy");
    auto service_proxy = std::static_pointer_cast<::trpc::test::unittest::GreeterServiceProxy>(mock_service_proxy_);
    greeter_client_ = std::make_unique<::test::unittest::GreeterClient>(service_proxy);
  }

  static void TearDownTestCase() {}

 protected:
  static std::unique_ptr<::test::unittest::GreeterClient> greeter_client_;
  static MockGreeterServiceProxyPtr mock_service_proxy_;
};

std::unique_ptr<::test::unittest::GreeterClient> GreeterClientTestFixture::greeter_client_ = nullptr;
MockGreeterServiceProxyPtr GreeterClientTestFixture::mock_service_proxy_ = nullptr;

TEST_F(GreeterClientTestFixture, SayHelloOK) {
  EXPECT_CALL(*mock_service_proxy_, SayHello(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(::trpc::kSuccStatus));
  EXPECT_TRUE(greeter_client_->SayHello());
}

TEST_F(GreeterClientTestFixture, AsyncSayHelloOK) {
  ::trpc::test::unittest::HelloReply rep;
  rep.set_msg("trpc");
  EXPECT_CALL(*mock_service_proxy_, AsyncSayHello(::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(
          ::testing::ByMove(::trpc::MakeReadyFuture<::trpc::test::unittest::HelloReply>(std::move(rep)))));

  auto fut = greeter_client_->AsyncSayHello().Then([](::trpc::Future<>&& fut) {
    EXPECT_TRUE(fut.IsReady());
    return ::trpc::MakeReadyFuture<>();
  });

  ::trpc::future::BlockingGet(std::move(fut));
}

}  // namespace testing
}  // namespace unittest
}  // namespace test

DEFINE_string(client_config, "trpc_cpp.yaml", "config file");

void ParseConfig(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::CommandLineFlagInfo info;
  if (GetCommandLineFlagInfo("client_config", &info) && info.is_default) {
    std::cerr << "start with config, for example: " << argv[0] << " --client_config=filepath" << std::endl;
    exit(-1);
  }

  std::cout << "FLAGS_config:" << FLAGS_client_config << std::endl;

  int ret = ::trpc::TrpcConfig::GetInstance()->Init(FLAGS_client_config);
  if (ret != 0) {
    std::cerr << "load config failed." << std::endl;
    exit(-1);
  }
}

int main(int argc, char* argv[]) {
  ParseConfig(argc, argv);

  return ::trpc::RunInTrpcRuntime([argc, argv]() mutable {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
  });
}
