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

#pragma once

#include <string>

#include "trpc/common/config/trpc_config.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/runtime/fiber_runtime.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_reactor.h"
#include "trpc/runtime/threadmodel/thread_model_manager.h"
#include "trpc/util/latch.h"

void InitFiberThreadModel(const std::string& path = "") {
  std::string conf_path = path;
  if (conf_path.empty()) {
    conf_path = "./trpc/transport/client/fiber/testing/fiber_transport_test.yaml";
  }
  trpc::TrpcConfig::GetInstance()->Init(conf_path);
  signal(SIGPIPE, SIG_IGN);
  trpc::fiber::StartRuntime();
}

void DestroyFiberThreadModel() { trpc::fiber::TerminateRuntime(); }

#define TEST_WITH_FIBER_THREAD_MAIN(path)   \
  int main(int argc, char** argv) {         \
    testing::InitGoogleTest(&argc, argv);   \
    InitFiberThreadModel(path);             \
    trpc::Latch l(1);                       \
    int ret = 0;                            \
    trpc::StartFiberDetached([&ret, &l]() { \
      trpc::StartAllFiberReactor();         \
      ret = RUN_ALL_TESTS();                \
      trpc::StopAllFiberReactor();          \
      trpc::JoinAllFiberReactor();          \
      l.count_down();                       \
    });                                     \
    l.wait();                               \
    DestroyFiberThreadModel();              \
    return ret;                             \
  }

#define TEST_WITH_FIBER_MAIN TEST_WITH_FIBER_THREAD_MAIN("")
