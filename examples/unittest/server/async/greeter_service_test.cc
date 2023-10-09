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

#include "examples/unittest/server/async/greeter_service.h"

#include <iostream>
#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "gflags/gflags.h"

#include "trpc/codec/trpc/trpc_server_codec.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/server/make_server_context.h"
#include "trpc/server/testing/mock_server_transport.h"
#include "trpc/util/thread/latch.h"

#include "examples/unittest/helloworld.trpc.pb.h"

namespace test {
namespace unittest {
namespace testing {

class GreeterServiceTestFixture : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    ::trpc::serialization::Init();
    transport_ = std::make_unique<::trpc::testing::MockServerTransport>();
    test_service_ = std::make_unique<GreeterServiceImpl>();
    test_service_->SetServerTransport(transport_.get());
  }

  static void TearDownTestCase() { ::trpc::serialization::Destroy(); }

 protected:
  static std::unique_ptr<GreeterServiceImpl> test_service_;
  static std::unique_ptr<::trpc::testing::MockServerTransport> transport_;
};

std::unique_ptr<GreeterServiceImpl> GreeterServiceTestFixture::test_service_ = nullptr;
std::unique_ptr<::trpc::testing::MockServerTransport> GreeterServiceTestFixture::transport_ = nullptr;

// test asynchronous response ok
TEST_F(GreeterServiceTestFixture, SayHelloOK) {
  ::trpc::Latch latch(1);
  EXPECT_CALL(*transport_, SendMsg(::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Invoke([&latch](::trpc::ServerContextPtr& context, ::trpc::NoncontiguousBuffer&& buffer) -> int {
        EXPECT_TRUE(context->GetStatus().OK());
        EXPECT_TRUE(!buffer.Empty());
        latch.count_down();
        return 0;
      }));

  ::trpc::TrpcServerCodec codec;
  auto server_context = ::trpc::testing::MakeServerContext(test_service_.get(), &codec);
          EXPECT_TRUE(server_context->GetStatus().OK());

  ::trpc::test::unittest::HelloRequest request;
  ::trpc::test::unittest::HelloReply reply;
  request.set_msg("trpc");
  test_service_->SayHello(server_context, &request, &reply);
  latch.wait();
}

// test asynchronous response ok multi times
TEST_F(GreeterServiceTestFixture, SayHelloOKMultiTimes) {
  int kCallTimes = 2;
  EXPECT_CALL(*transport_, SendMsg(::testing::_, ::testing::_))
      .Times(kCallTimes)
      .WillRepeatedly(::testing::Invoke([](::trpc::ServerContextPtr& context, ::trpc::NoncontiguousBuffer&& buffer) -> int {
        EXPECT_TRUE(context->GetStatus().OK());
        EXPECT_TRUE(!buffer.Empty());
        return 0;
      }));

  for(int i = 0; i < kCallTimes; i++) {
    ::trpc::TrpcServerCodec codec;
    ::trpc::Latch latch(1);
    auto server_context = ::trpc::testing::MakeServerContext(test_service_.get(), &codec);
    EXPECT_TRUE(server_context->GetStatus().OK());
    server_context->SetSendMsgCallback([&latch]() { latch.count_down(); });
    ::trpc::test::unittest::HelloRequest request;
    ::trpc::test::unittest::HelloReply reply;
    request.set_msg("trpc");
    test_service_->SayHello(server_context, &request, &reply);
    latch.wait();
  }
}

}  // namespace testing
}  // namespace unittest
}  // namespace test

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
