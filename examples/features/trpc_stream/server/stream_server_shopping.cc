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

#include <string>
#include <utility>

#include "trpc/common/logging/trpc_logging.h"
#include "trpc/common/trpc_app.h"

#include "examples/features/trpc_stream/server/stream_service_shopping.h"

namespace test {
namespace shopping {

class ShoppingStreamServer : public ::trpc::TrpcApp {
 public:
  int Initialize() override {
    const auto& config = ::trpc::TrpcConfig::GetInstance()->GetServerConfig();
    std::string service_name = fmt::format("{}.{}.{}.{}", "trpc", config.app, config.server, "StreamShopping");
    TRPC_FMT_INFO("service name:{}", service_name);
    RegisterService(service_name, std::make_shared<::test::shopping::StreamShoppingServiceImpl>());

    service_name = fmt::format("{}.{}.{}.{}", "trpc", config.app, config.server, "RawDataStreamService");
    TRPC_FMT_INFO("service name:{}", service_name);
    RegisterService(service_name, std::make_shared<::test::shopping::RawDataStreamService>());

    return 0;
  }

  void Destroy() override {}
};

}  // namespace shopping
}  // namespace test

int main(int argc, char** argv) {
  test::shopping::ShoppingStreamServer server;

  server.Main(argc, argv);
  server.Wait();

  return 0;
}
