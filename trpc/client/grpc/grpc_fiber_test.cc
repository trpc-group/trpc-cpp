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

#include <vector>

#include "gtest/gtest.h"

#include "trpc/client/grpc/testing/grpc_client_call_testing.h"
#include "trpc/client/trpc_client.h"
#include "trpc/proto/testing/helloworld.pb.h"
#include "trpc/server/testing/fiber_server_testing.h"
#include "trpc/server/testing/greeter_service_testing.h"

namespace trpc::testing {

class GrpcCallTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    std::string config_path = "trpc/client/grpc/testing/grpc_fiber.yaml";
    ASSERT_TRUE(Server::SetUp(config_path));
  }

  static void TearDownTestCase() {
    Server::TearDown();
    trpc::GetTrpcClient()->Stop();
    trpc::GetTrpcClient()->Destroy();
  }

  class Server : public TestFiberServer {
    bool Init() override {
      // Server.
      ServicePtr greeter = std::make_shared<trpc::test::helloworld::GreeterServiceTest>();
      bool ok = RegisterService("http.test.helloworld.Greeter", greeter);
      if (!ok) {
        std::cerr << "failed to register service" << std::endl;
        return false;
      }

      // Client.
      auto greeter_client =
          trpc::GetTrpcClient()->GetProxy<trpc::test::helloworld::GreeterServiceProxy>("http.test.helloworld.Greeter");
      if (!greeter_client) {
        std::cerr << "failed to get client" << std::endl;
        return false;
      }

      std::vector<TestFiberServer::testing_args_t> testings{
          // Executing multiple test cases is to verify the logic of handling concurrent requests.
          {"grpc unary rpc call-1", [greeter_client]() { return TestGrpcClientUnaryRpcCall(greeter_client); }, false},
          {"grpc unary rpc call-2", [greeter_client]() { return TestGrpcClientUnaryRpcCall(greeter_client); }, false},
          {"grpc unary rpc call-3", [greeter_client]() { return TestGrpcClientUnaryRpcCall(greeter_client); }, false},
      };
      AddTestings(std::move(testings));
      return true;
    }

    void Destroy() override {}
  };
};
/*
TEST_F(GrpcCallTest, ServerStartAndServe) {
  auto server = std::make_shared<GrpcCallTest::Server>();
  bool ok = server->Run();
  ASSERT_TRUE(ok);
}
*/
}  // namespace trpc::testing
