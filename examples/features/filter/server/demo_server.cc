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

#include <memory>
#include <string>

#include "fmt/format.h"

#include "trpc/common/logging/trpc_logging.h"
#include "trpc/common/status.h"
#include "trpc/common/trpc_app.h"

#include "examples/features/filter/common/invoke_stat_filter.h"
#include "examples/features/filter/common/user_rpc_filter.h"
#include "examples/helloworld/helloworld.trpc.pb.h"

namespace examples::filter {

class GreeterServiceImpl : public ::trpc::test::helloworld::Greeter {
 public:
  ::trpc::Status SayHello(::trpc::ServerContextPtr context, const ::trpc::test::helloworld::HelloRequest* request,
                          ::trpc::test::helloworld::HelloReply* reply) {
    std::string response = "Hello, " + request->msg();
    reply->set_msg(response);

    return ::trpc::kSuccStatus;
  }
};

class HelloWorldServer : public ::trpc::TrpcApp {
 public:
  int Initialize() override {
    const auto& config = ::trpc::TrpcConfig::GetInstance()->GetServerConfig();
    std::string service_name = fmt::format("{}.{}.{}.{}", "trpc", config.app, config.server, "Greeter");
    TRPC_FMT_INFO("service name:{}", service_name);
    ::trpc::ServicePtr greeter_service(std::make_shared<GreeterServiceImpl>());
    RegisterService(service_name, greeter_service);

    return 0;
  }

  // Register filters you need here if your program is a server.
  int RegisterPlugins() override {
    // Register user rpc server filter
    ::trpc::MessageServerFilterPtr pb_rpc_filter = std::make_shared<examples::filter::UserGreeterPbRpcServerFilter>();
    ::trpc::TrpcPlugin::GetInstance()->RegisterServerFilter(pb_rpc_filter);

    ::trpc::MessageServerFilterPtr invoke_stat_filter = std::make_shared<examples::filter::ServerInvokeStatFilter>();
    ::trpc::TrpcPlugin::GetInstance()->RegisterServerFilter(invoke_stat_filter);

    return 0;
  }

  void Destroy() override {}
};

}  // namespace examples::filter

int main(int argc, char** argv) {
  examples::filter::HelloWorldServer helloworld_server;

  helloworld_server.Main(argc, argv);
  helloworld_server.Wait();

  return 0;
}
