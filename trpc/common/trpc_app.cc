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

#include "trpc/common/trpc_app.h"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <atomic>
#include <iostream>
#include <string>
#include <utility>

#include "gflags/gflags.h"

// #include "trpc/client/http/http_service_proxy.h"
#include "trpc/filter/server_filter_manager.h"
#include "trpc/runtime/runtime.h"
#include "trpc/tvar/common/sampler.h"
#include "trpc/util/chrono/chrono.h"
#include "trpc/util/time.h"

namespace trpc {

DEFINE_bool(trpc_version, false, "trpc cpp version flag");
DEFINE_string(config, "trpc_cpp_default.yaml", "trpc cpp framework config file");
DEFINE_bool(daemon, false, "run in daemon mode");

static std::atomic_bool terminate_;

int TrpcApp::Main(int argc, char* argv[]) {
  ParseFrameworkConfig(argc, argv);

  signal(SIGUSR1, TrpcApp::SigUsr1Handler);
  signal(SIGUSR2, TrpcApp::SigUsr2Handler);

  // init logging
  TRPC_ASSERT(log::Init());

  InitializeRuntime();

  return 0;
}

void TrpcApp::Wait() { DestroyRuntime(); }

void TrpcApp::Terminate() { terminate_.store(true, std::memory_order_release); }

bool TrpcApp::CheckTerminated() { return terminate_.load(std::memory_order_acquire); }

void TrpcApp::InitializeRuntime() {
  client_ = trpc::GetTrpcClient();

  server_ = trpc::GetTrpcServer();
  server_->SetTerminateFunction([]() -> bool { return terminate_.load(std::memory_order_acquire); });

  if (!runtime::StartRuntime()) {
    std::cerr << "StartRuntime Failed and Exit." << std::endl;
    TRPC_LOG_CRITICAL("StartRuntime Failed and Exit.");
    return;
  }

  this->Run();
}

void TrpcApp::Run() {
  auto exe_func = [this] { this->Execute(); };

  if (!runtime::Run(std::move(exe_func))) {
    std::cerr << "Run Failed and Exit." << std::endl;
    TRPC_LOG_CRITICAL("Run Failed and Exit.");
  }
}

void TrpcApp::DestroyRuntime() {
  runtime::TerminateRuntime();

  server_->Destroy();

  trpc::GetTrpcClient()->Destroy();

  TrpcPlugin::GetInstance()->DestroyResource();
}

void TrpcApp::Execute() {
  uint64_t begin_time = trpc::time::GetMilliSeconds();

  TrpcPlugin::GetInstance()->SetInvokeByFramework();

  // register user custom plugin
  RegisterPlugins();

  // register framework inner plugin
  TrpcPlugin::GetInstance()->RegisterPlugins();

  TRPC_ASSERT(server_->Initialize());
  server_->SetTerminateFunction([]() -> bool { return terminate_.load(std::memory_order_acquire); });

  int ret = Initialize();
  if (ret != 0) {
    std::cerr << "Initialize Failed and Terminate Server." << std::endl;
    TRPC_LOG_CRITICAL("Initialize Failed and Terminate Server.");

    terminate_.store(true, std::memory_order_release);
  } else {
    bool is_server_start_success = server_->Start();
    if (!is_server_start_success) {
      std::cerr << "TrpcServer Start failed." << std::endl;
      TRPC_LOG_CRITICAL("TrpcServer Start failed.");

      terminate_.store(true, std::memory_order_release);
    } else {
      std::cout << "Server InitializeRuntime use time:" << (trpc::time::GetMilliSeconds() - begin_time) << "(ms)"
                << std::endl;

      server_->WaitForShutdown();
    }
  }

  Destroy();

  TrpcPlugin::GetInstance()->UnregisterPlugins();
}

void TrpcApp::ParseFrameworkConfig(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, false);

  if (FLAGS_trpc_version) {
    std::cout << "tRPC-Cpp Version:" << TRPC_Cpp_Version() << std::endl;
    exit(0);
  }

  if (FLAGS_daemon) {
    tvar::SamplerCollectorThreadStop();

    if (!Daemonlize()) exit(-1);

    tvar::SamplerCollectorThreadStart();
  }

  google::CommandLineFlagInfo info;

  if (google::GetCommandLineFlagInfo("config", &info) && info.is_default) {
    if (InitFrameworkConfig() == 0) {
      return;
    }

    std::cerr << "start server with config, for example: " << argv[0] << " --config=default.yaml" << std::endl;
    exit(-1);
  }

  int ret = TrpcConfig::GetInstance()->Init(FLAGS_config);
  if (ret != 0) {
    std::cerr << "load config failed." << std::endl;
    exit(-1);
  }

  UpdateFrameworkConfig();
}

bool TrpcApp::Daemonlize() {
  if (daemon(1, 1) != 0) {
    std::cerr << "daemon failed: " << strerror(errno) << std::endl;
    return false;
  }
  return true;
}

void TrpcApp::SigUsr1Handler(int signo) {
  bool ret = ConfigHelper::GetInstance()->Reload();
  if (ret) {
    TRPC_LOG_INFO("reload config ok");
  } else {
    TRPC_LOG_ERROR("failed to reload config");
  }

  signal(SIGUSR1, TrpcApp::SigUsr1Handler);
}

void TrpcApp::SigUsr2Handler(int signo) {
  terminate_.store(true, std::memory_order_release);

  signal(SIGUSR2, TrpcApp::SigUsr2Handler);
}

TrpcServer::RegisterRetCode TrpcApp::RegisterService(const std::string& service_name, ServicePtr& service,
                                                     bool auto_start) {
  TRPC_CHECK(service, "service is NOT allowed empty");
  return server_->RegisterService(service_name, service, auto_start);
}

TrpcServer::RegisterRetCode TrpcApp::RegisterService(const std::string& service_name, const ServicePtr& service,
                                                     bool auto_start) {
  TRPC_CHECK(service, "service is NOT allowed empty");
  return server_->RegisterService(service_name, const_cast<ServicePtr&>(service), auto_start);
}

bool TrpcApp::StartService(const std::string& service_name) { return server_->StartService(service_name); }

bool TrpcApp::StartService(const ServiceConfig& config, ServicePtr& service) {
  return server_->StartService(config, service);
}

bool TrpcApp::StopService(const std::string& service_name, bool clean_conn) {
  return server_->StopService(service_name, clean_conn);
}

void TrpcApp::RegisterCmd(trpc::http::OperationType type, const std::string& url, trpc::AdminHandlerBase* handler) {
  auto admin_service = server_->GetAdminService();
  TRPC_ASSERT(admin_service != nullptr);

  admin_service->RegisterCmd(type, url, handler);
}

void TrpcApp::RegisterCmd(trpc::http::OperationType type, const std::string& url,
                          const std::shared_ptr<trpc::AdminHandlerBase>& handler) {
  auto admin_service = server_->GetAdminService();
  TRPC_ASSERT(admin_service != nullptr);

  admin_service->RegisterCmd(type, url, handler);
}

void TrpcApp::RegisterConfigUpdateNotifier(const std::string& notify_name,
                                           const std::function<void(const YAML::Node&)>& notify_cb) {
  ConfigHelper::GetInstance()->RegisterConfigUpdateNotifier(notify_name, notify_cb);
}

}  // namespace trpc
