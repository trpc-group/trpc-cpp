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

#include <functional>
#include <memory>
#include <string>

#include "trpc/common/trpc_app.h"
#include "trpc/log/trpc_log.h"
#include "trpc/server/rpc/rpc_method_handler.h"
#include "trpc/server/rpc/rpc_service_impl.h"
#include "trpc/server/rpc/rpc_service_method.h"

namespace examples::noop {

class DemoServiceImpl : public ::trpc::RpcServiceImpl {
 public:
  DemoServiceImpl() {
    auto handler1 = new ::trpc::RpcMethodHandler<std::string, std::string>(
        std::bind(&DemoServiceImpl::NoopSayHello1, this, std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3));
    AddRpcServiceMethod(new ::trpc::RpcServiceMethod("NoopSayHello1", ::trpc::MethodType::UNARY, handler1));

    auto handler2 = new ::trpc::RpcMethodHandler<::trpc::NoncontiguousBuffer, ::trpc::NoncontiguousBuffer>(
        std::bind(&DemoServiceImpl::NoopSayHello2, this, std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3));
    AddRpcServiceMethod(new ::trpc::RpcServiceMethod("NoopSayHello2", ::trpc::MethodType::UNARY, handler2));
  }

  ::trpc::Status NoopSayHello1(const ::trpc::ServerContextPtr& context,
                               const std::string* request,
                               std::string* reply) {
    TRPC_FMT_INFO("request msg: {}", *request);
    *reply = *request;
    return ::trpc::kSuccStatus;
  }

  ::trpc::Status NoopSayHello2(const ::trpc::ServerContextPtr& context,
                               const ::trpc::NoncontiguousBuffer* request,
                               ::trpc::NoncontiguousBuffer* reply) {
    TRPC_FMT_INFO("request msg: {}", ::trpc::FlattenSlow(*request));
    *reply = *request;
    return ::trpc::kSuccStatus;
  }
};

class DemoServer : public ::trpc::TrpcApp {
 public:
  int Initialize() override {
    const auto& config = ::trpc::TrpcConfig::GetInstance()->GetServerConfig();
    // Set the service name, which must be the same as the value of the `server:service:name` configuration item
    // in the framework configuration file, otherwise the framework cannot receive requests normally
    std::string service_name = fmt::format("{}.{}.{}.{}", "trpc", config.app, config.server, "demo_service");

    TRPC_FMT_INFO("service name:{}", service_name);

    RegisterService(service_name, std::make_shared<DemoServiceImpl>());

    return 0;
  }

  void Destroy() override {}
};

}  // namespace examples::noop

int main(int argc, char** argv) {
  examples::noop::DemoServer demo_server;
  demo_server.Main(argc, argv);
  demo_server.Wait();

  return 0;
}
