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

namespace examples::json {

class DemoServiceImpl : public ::trpc::RpcServiceImpl {
 public:
  DemoServiceImpl() {
    auto handler = new ::trpc::RpcMethodHandler<rapidjson::Document, rapidjson::Document>(
        std::bind(&DemoServiceImpl::JsonSayHello, this, std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3));
    AddRpcServiceMethod(
        new ::trpc::RpcServiceMethod("JsonSayHello", ::trpc::MethodType::UNARY, handler));
  }

  ::trpc::Status JsonSayHello(const ::trpc::ServerContextPtr& context,
                              const rapidjson::Document* request,
                              rapidjson::Document* reply) {
    for (rapidjson::Value::ConstMemberIterator iter = request->MemberBegin(); iter != request->MemberEnd(); ++iter) {
      TRPC_FMT_INFO("json name: {}, value: {}", iter->name.GetString(), iter->value.GetInt());
    }
    reply->CopyFrom(*request, const_cast<rapidjson::Document*>(request)->GetAllocator());
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

}  // namespace examples::json

int main(int argc, char** argv) {
  examples::json::DemoServer demo_server;
  demo_server.Main(argc, argv);
  demo_server.Wait();

  return 0;
}
