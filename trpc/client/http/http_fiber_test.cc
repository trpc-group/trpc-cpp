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

#include "trpc/client/http/http_service_proxy.h"

#include <vector>

#include "gtest/gtest.h"

#include "trpc/client/http/testing/http_client_call_testing.h"
#include "trpc/client/trpc_client.h"
#include "trpc/proto/testing/helloworld.pb.h"
#include "trpc/server/testing/fiber_server_testing.h"
#include "trpc/server/testing/greeter_service_testing.h"
#include "trpc/server/testing/http_service_testing.h"
#ifdef TRPC_BUILD_INCLUDE_SSL
#include "trpc/transport/common/ssl/ssl.h"
#endif
namespace trpc::testing {

class HttpCallTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    std::string config_path = "trpc/client/http/testing/http_fiber.yaml";
#ifdef TRPC_BUILD_INCLUDE_SSL
    ASSERT_TRUE(ssl::InitOpenSsl());
    config_path = "trpc/client/http/testing/http_fiber_with_ssl.yaml";
#endif
    ASSERT_TRUE(Server::SetUp(config_path));
  }

  static void TearDownTestCase() {
    Server::TearDown();
#ifdef TRPC_BUILD_INCLUDE_SSL
    ssl::DestroyOpenSsl();
#endif
    trpc::GetTrpcClient()->Stop();
    trpc::GetTrpcClient()->Destroy();
  }

  class Server : public TestFiberServer {
    bool Init() override {
      // Server.
      ServicePtr greeterOverHttp = std::make_shared<trpc::test::helloworld::GreeterServiceTest>();
      if (!RegisterService("trpc.test.helloworld.GreeterOverHTTP", greeterOverHttp)) {
        std::cerr << "failed to register service" << std::endl;
        return false;
      }

      auto http_service = std::make_shared<trpc::HttpService>();
      http_service->SetRoutes(SetHttpRoutes);
      trpc::ServicePtr default_http_service{http_service};
      if (!RegisterService("http.test.helloworld", default_http_service)) {
        std::cerr << "failed to register service" << std::endl;
        return false;
      }

      // Client.
      auto test_http_client = trpc::GetTrpcClient()->GetProxy<trpc::http::HttpServiceProxy>("test_http_client");
      if (!test_http_client) {
        std::cerr << "failed to get client" << std::endl;
        return false;
      }
      auto test_greeter_client = trpc::GetTrpcClient()->GetProxy<trpc::http::HttpServiceProxy>("test_greeter_client");
      if (!test_greeter_client) {
        std::cerr << "failed to get client" << std::endl;
        return false;
      }

      std::vector<TestFiberServer::testing_args_t> testings{
          {"http unary rpc call", [test_greeter_client]() { return TestHttpClientUnaryRpcCall(test_greeter_client); },
           false},
          {"http unary call", [test_http_client]() { return TestHttpClientUnaryCall(test_http_client); }, false},
          {"http streaming call", [test_http_client]() { return TestHttpClientStreamingCall(test_http_client); },
           false},
      };
      AddTestings(std::move(testings));
      return true;
    }

    void Destroy() override {}
  };
};

TEST_F(HttpCallTest, ServerStartAndServe) {
  auto server = std::make_shared<HttpCallTest::Server>();
  bool ok = server->Run();
  ASSERT_TRUE(ok);
}

}  // namespace trpc::testing
