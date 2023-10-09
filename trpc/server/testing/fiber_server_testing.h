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

#include <vector>

#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/naming/trpc_naming_registry.h"
#include "trpc/runtime/runtime.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/server/testing/greeter_service_testing.h"
#include "trpc/server/trpc_server.h"
#include "trpc/stream/stream_handler_manager.h"
#include "trpc/transport/common/connection_handler_manager.h"
#include "trpc/transport/common/io_handler_manager.h"

namespace trpc::testing {

class TestFiberServer {
 public:
  struct testing_args_t {
    std::string testing_name;
    std::function<bool()> testing_case_executor;
    // Final return code of `testing_case_executor`.
    bool ok;
  };

  virtual bool Init() = 0;
  virtual void Destroy() {}

  void AddTestings(std::vector<testing_args_t>&& testings) { testings_ = std::move(testings); }
  bool RegisterService(const std::string& service_name, ServicePtr& service) {
    TrpcServer::RegisterRetCode ret_code = server_->RegisterService(service_name, service);
    return (ret_code == TrpcServer::RegisterRetCode::kOk);
  }

  bool Run() {
    bool ret = runtime::Run([this]() {
      std::shared_ptr<TrpcServer> server = trpc::GetTrpcServer();
      if (!server->Initialize()) {
        ASSERT_TRUE(false && "failed to init server");
        return;
      }
      this->server_ = server;
      server->SetTerminateFunction([this]() { return this->GetTerminate(); });

      if (!Init()) {
        ASSERT_TRUE(false && "failed to init service");
        return;
      }

      bool final_ok{true};
      trpc::FiberLatch l{1};
      trpc::StartFiberDetached([this, &server, &final_ok, &l]() {
        while (server->GetServerState() != TrpcServer::ServerState::kStart) {
          std::cout << "waits for server to start." << std::endl;
          trpc::FiberSleepFor(std::chrono::milliseconds(1000));
        }

        auto wg_count = static_cast<std::ptrdiff_t>(testings_.size());
        trpc::FiberLatch testings_wg{wg_count};
        for (auto& t : this->testings_) {
          trpc::StartFiberDetached([&testings_wg, &t]() {
            t.ok = t.testing_case_executor();
            testings_wg.CountDown();
          });
        }

        testings_wg.Wait();
        for (const auto& t : testings_) {
          final_ok &= t.ok;
          std::cerr << "testing name: " << t.testing_name << ", ok: " << t.ok << std::endl;
        }
        std::cerr << "final result of testings: " << final_ok << std::endl;

        SetTerminate();
        l.CountDown();
      });

      server->Start();
      server->WaitForShutdown();
      l.Wait();
      server->Destroy();
      Destroy();
      ASSERT_TRUE(final_ok);
    });
    return ret;
  }

  static bool SetUp(const std::string& config_path) {
    TRPC_ASSERT((TrpcConfig::GetInstance()->Init(config_path) == 0));
    TRPC_ASSERT(codec::Init());
    TRPC_ASSERT(serialization::Init());
    TRPC_ASSERT(naming::Init());
    TRPC_ASSERT(InitIoHandler());
    TRPC_ASSERT(InitConnectionHandler());
    TRPC_ASSERT(stream::InitStreamHandler());
    runtime::StartRuntime();
    return true;
  }

  static void TearDown() {
    runtime::TerminateRuntime();
    stream::DestroyStreamHandler();
    DestroyIoHandler();
    DestroyConnectionHandler();
    naming::Stop();
    naming::Destroy();
    serialization::Destroy();
    codec::Destroy();
  }

 private:
  bool GetTerminate() { return terminate_.load(std::memory_order_acquire); }
  void SetTerminate() { terminate_.store(true, std::memory_order_release); }

  std::atomic<bool> terminate_{false};
  std::vector<testing_args_t> testings_;
  std::shared_ptr<TrpcServer> server_{nullptr};
};

}  // namespace trpc::testing
