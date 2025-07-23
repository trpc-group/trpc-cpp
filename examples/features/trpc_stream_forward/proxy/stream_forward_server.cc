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

#include <string>
#include <utility>

#include "trpc/common/logging/trpc_logging.h"
#include "trpc/common/trpc_app.h"

#include "examples/features/trpc_stream_forward/proxy/stream_forward_service.h"

namespace test {
namespace helloworld {

class HelloWorldStreamServer : public ::trpc::TrpcApp {
 public:
  int Initialize() override {
    const auto& config = ::trpc::TrpcConfig::GetInstance()->GetServerConfig();
    std::string service_name = fmt::format("{}.{}.{}.{}", "trpc", config.app, config.server, "StreamForward");
    TRPC_FMT_INFO("service name:{}", service_name);
    RegisterService(service_name, std::make_shared<::test::helloworld::StreamForwardImpl>());

    return 0;
  }

  void Destroy() override {}
};

}  // namespace helloworld
}  // namespace test

int main(int argc, char** argv) {
  test::helloworld::HelloWorldStreamServer server;

  server.Main(argc, argv);
  server.Wait();

  return 0;
}
