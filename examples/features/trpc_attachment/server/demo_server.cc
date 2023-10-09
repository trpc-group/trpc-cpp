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

#include <memory>
#include <string>

#include "fmt/format.h"

#include "trpc/common/trpc_app.h"

#include "examples/helloworld/helloworld.trpc.pb.h"

namespace examples::attachment {

class GreeterServiceImpl : public ::trpc::test::helloworld::Greeter {
 public:
  ::trpc::Status SayHello(::trpc::ServerContextPtr context,
                          const ::trpc::test::helloworld::HelloRequest* request,
                          ::trpc::test::helloworld::HelloReply* reply) override {
    // Your can access more information from rpc context, eg: remote ip and port
    TRPC_FMT_INFO("remote address: {}:{}", context->GetIp(), context->GetPort());
    TRPC_FMT_INFO("request message: {}", request->msg());

    ::trpc::NoncontiguousBuffer req_atta = context->GetRequestAttachment();
    std::string rsp_str = ::trpc::FlattenSlow(req_atta);
    TRPC_FMT_INFO("request attachment: {}", rsp_str);

    std::string rsp_msg = "hello, " + request->msg();
    reply->set_msg(rsp_msg);

    ::trpc::NoncontiguousBuffer rsp_atta = ::trpc::CreateBufferSlow("return " + rsp_str);
    context->SetResponseAttachment(std::move(rsp_atta));

    return ::trpc::kSuccStatus;
  }
};

class DemoServer : public ::trpc::TrpcApp {
 public:
  int Initialize() override {
    const auto& config = ::trpc::TrpcConfig::GetInstance()->GetServerConfig();
    // Set the service name, which must be the same as the value of the `/server/service/name` configuration item
    // in the configuration file, otherwise the framework cannot receive requests normally.
    std::string service_name = fmt::format("{}.{}.{}.{}", "trpc", config.app, config.server, "Greeter");
    TRPC_FMT_INFO("service name:{}", service_name);

    RegisterService(service_name, std::make_shared<GreeterServiceImpl>());

    return 0;
  }

  void Destroy() override {}
};

}  // namespace examples::attachment

int main(int argc, char** argv) {
  examples::attachment::DemoServer demo_server;

  demo_server.Main(argc, argv);
  demo_server.Wait();

  return 0;
}
