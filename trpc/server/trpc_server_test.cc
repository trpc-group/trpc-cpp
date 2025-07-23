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

#include "trpc/server/trpc_server.h"

#include <atomic>
#include <chrono>
#include <deque>
#include <iostream>
#include <memory>
#include <random>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/codec/trpc/testing/trpc_protocol_testing.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/naming/testing/trpc_registry_testing.h"
#include "trpc/naming/registry_factory.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/separate_runtime.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/server/testing/greeter_service_testing.h"

namespace trpc::testing {

class TrpcServerTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    EXPECT_EQ(TrpcConfig::GetInstance()->Init("trpc/server/testing/merge_server.yaml"), 0);

    codec::Init();
    serialization::Init();

    test_trpc_registry = trpc::MakeRefCounted<TestTrpcRegistry>();

    RegistryPtr test_registry = test_trpc_registry;

    RegistryFactory::GetInstance()->Register(test_registry);

    trpc::merge::StartRuntime();
    trpc::separate::StartAdminRuntime();
  }

  static void TearDownTestCase() {
    trpc::separate::TerminateAdminRuntime();
    trpc::merge::TerminateRuntime();

    RegistryFactory::GetInstance()->Clear();

    codec::Destroy();
    serialization::Destroy();
  }
 public:
  static RefPtr<TestTrpcRegistry> test_trpc_registry;
};

RefPtr<TestTrpcRegistry> TrpcServerTest::test_trpc_registry;

TEST_F(TrpcServerTest, Normal) {
  std::atomic<bool> exited{false};
  std::shared_ptr<TrpcServer> server = GetTrpcServer();

  std::thread th([&] {
    ASSERT_TRUE(server->Initialize());
    server->SetTerminateFunction([&]() { return exited.load(std::memory_order_acquire); });

    ServicePtr greeter_service_test = std::make_shared<trpc::test::helloworld::GreeterServiceTest>();
    TrpcServer::RegisterRetCode ret_code = server->RegisterService("test_greeter_service",
        greeter_service_test);
    ASSERT_TRUE(ret_code == TrpcServer::RegisterRetCode::kOk);

    ASSERT_TRUE(test_trpc_registry->GetRegisterFlag());

    std::cout << "register ok" << std::endl;

    server->Start();
    std::cout << "start ok" << std::endl;
    server->WaitForShutdown();
    std::cout << "shutdown ok" << std::endl;
    server->Destroy();

    ASSERT_TRUE(test_trpc_registry->GetUnRegisterFlag());
  });

  while (server->GetServerState() != TrpcServer::ServerState::kStart) {
    usleep(100000);
  }

  NetworkAddress addr("127.0.0.1", 13451);

  Socket socket = Socket::CreateTcpSocket(addr.IsIpv6());

  int ret = socket.Connect(addr);
  ASSERT_TRUE(ret == 0);

  DummyTrpcProtocol req_data;
  req_data.func = "/trpc.test.helloworld.Greeter/SayHello";

  trpc::test::helloworld::HelloRequest hello_req;
  hello_req.set_msg("hello world");

  NoncontiguousBuffer req_bin_data;

  ASSERT_TRUE(PackTrpcRequest(req_data, static_cast<void*>(&hello_req), req_bin_data));

  std::cout << "pack ok" << std::endl;

  char* message = new char[64 * 1024];

  FlattenToSlow(req_bin_data, message, req_bin_data.ByteSize());

  int send_size = socket.Send(message, req_bin_data.ByteSize());
  ASSERT_TRUE(send_size == static_cast<int>(req_bin_data.ByteSize()));

  char recvbuf[64 * 1024] = {0};
  int recv_size = socket.Recv(recvbuf, sizeof(recvbuf));
  ASSERT_TRUE(recv_size > 0);

  NoncontiguousBuffer rsp_bin_data = CreateBufferSlow(recvbuf, recv_size);

  trpc::test::helloworld::HelloReply hello_rsp;

  ASSERT_TRUE(UnPackTrpcResponse(rsp_bin_data, req_data, &hello_rsp));

  ASSERT_TRUE(hello_rsp.msg() == hello_req.msg());

  exited.store(true, std::memory_order_release);

  th.join();
}

TEST_F(TrpcServerTest, RegisterServiceFailed) {
  std::atomic<bool> exited{false};
  std::shared_ptr<TrpcServer> server = GetTrpcServer();

  ASSERT_TRUE(server->Initialize());
  server->SetTerminateFunction([&]() { return exited.load(std::memory_order_acquire); });

  ServicePtr greeter_service_test = std::make_shared<trpc::test::helloworld::GreeterServiceTest>();

  // service name empty, register failed
  ASSERT_EQ(server->RegisterService("", greeter_service_test, false), TrpcServer::RegisterRetCode::kParameterError);

  // service empty, register failed
  ServicePtr service_no_exit = nullptr;
  ASSERT_EQ(server->RegisterService("test_service", service_no_exit, false),
      TrpcServer::RegisterRetCode::kParameterError);

  // service name not found in xxx.yaml, register failed
  ASSERT_EQ(server->RegisterService("test_service_no_exit", greeter_service_test, false),
            TrpcServer::RegisterRetCode::kSearchError);

  server->Destroy();
}

}  // namespace trpc::testing
