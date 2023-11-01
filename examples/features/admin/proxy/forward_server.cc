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

#include <atomic>
#include <memory>
#include <string>

#include "fmt/format.h"

#include "examples/features/admin/proxy/custom_conf.h"
#include "examples/features/admin/proxy/forward_service.h"
#include "trpc/admin/admin_handler.h"
#include "trpc/common/trpc_app.h"
#include "trpc/tvar/tvar.h"

namespace examples::admin {

std::atomic<int> custom_value;
::trpc::tvar::Counter<uint64_t> counter("user/my_count");

/// @brief Implements a AdminHandler by overriding the CommandHandle interface of ::trpc::AdminHandlerBase.
class MyAdminHandler1 : public ::trpc::AdminHandlerBase {
 public:
  MyAdminHandler1() { description_ = "This is my first admin command"; }

  void CommandHandle(::trpc::http::HttpRequestPtr req, ::rapidjson::Value& result,
                     ::rapidjson::Document::AllocatorType& alloc) override {
    // add your processing logic here
    TRPC_LOG_INFO("execute the first admin command");

    // set the return result
    result.AddMember("custom_value", custom_value.load(std::memory_order_release), alloc);
  }
};

/// @brief Implements a AdminHandler by overriding the Handle interface of ::trpc::AdminHandlerBase.
class MyAdminHandler2 : public ::trpc::AdminHandlerBase {
 public:
  MyAdminHandler2() { description_ = "This is my second admin command"; }

  void CommandHandle(::trpc::http::HttpRequestPtr req, ::rapidjson::Value& result,
                     ::rapidjson::Document::AllocatorType& alloc) override {}

  ::trpc::Status Handle(const std::string& path, trpc::ServerContextPtr context, ::trpc::http::HttpRequestPtr req,
                        ::trpc::http::HttpResponse* rsp) override {
    // add your processing logic here
    TRPC_LOG_INFO("execute the second admin command");

    // set the return data in any format
    rsp->SetContent("custom_value: " + std::to_string(custom_value.load(std::memory_order_release)));
    return ::trpc::kSuccStatus;
  }
};

class ForwardServer : public ::trpc::TrpcApp {
 public:
  int Initialize() override {
    // register your own admin handler
    RegisterCmd(::trpc::http::OperationType::GET, "/cmds/myhandler1", std::make_shared<MyAdminHandler1>());
    RegisterCmd(::trpc::http::OperationType::GET, "/cmds/myhandler2", std::make_shared<MyAdminHandler2>());

    // load custom config
    CustomConfig custom_config;
    if (::trpc::ConfigHelper::GetInstance()->GetConfig({"custom"}, custom_config)) {
      custom_value.store(custom_config.value, std::memory_order_release);
    } else {
      TRPC_LOG_ERROR("load CustomConfig fail!");
      custom_value.store(-1, std::memory_order_release);
    }

    // register config update notifier
    RegisterConfigUpdateNotifier("notify", [](const YAML::Node& root) {
      TRPC_LOG_INFO("reload callback");
      CustomConfig custom_config;
      if (::trpc::ConfigHelper::GetInstance()->GetConfig(root, {"custom"}, custom_config)) {
        custom_value.store(custom_config.value, std::memory_order_release);
      } else {
        TRPC_LOG_ERROR("reload CustomConfig fail!");
      }
    });

    // use tvar (For more examples of using tvar, please refer to the examples/features/tvar)
    counter.Add(10);

    const auto& config = ::trpc::TrpcConfig::GetInstance()->GetServerConfig();
    // Set the service name, which must be the same as the value of the `server:service:name` configuration item
    // in the framework configuration file, otherwise the framework cannot receive requests normally
    std::string service_name = fmt::format("{}.{}.{}.{}", "trpc", config.app, config.server, "Forward");

    TRPC_FMT_INFO("service name:{}", service_name);

    RegisterService(service_name, std::make_shared<ForwardServiceImpl>());

    return 0;
  }

  void Destroy() override {}
};

}  // namespace examples::admin

int main(int argc, char** argv) {
  examples::admin::ForwardServer forward_server;
  forward_server.Main(argc, argv);
  forward_server.Wait();

  return 0;
}
