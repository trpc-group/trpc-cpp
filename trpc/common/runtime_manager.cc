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

#include "trpc/common/runtime_manager.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/compressor/trpc_compressor.h"
#include "trpc/config/trpc_conf.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/metrics/trpc_metrics.h"
#include "trpc/naming/trpc_naming_registry.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"
#include "trpc/runtime/fiber_runtime.h"
#include "trpc/runtime/runtime.h"
#include "trpc/runtime/runtime_state.h"
#include "trpc/runtime/separate_runtime.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/telemetry/telemetry_factory.h"
#include "trpc/telemetry/trpc_telemetry.h"
#include "trpc/tracing/trpc_tracing.h"
#include "trpc/transport/common/connection_handler_manager.h"
#include "trpc/transport/common/io_handler_manager.h"
#include "trpc/transport/common/ssl_helper.h"
#include "trpc/util/buffer/memory_pool/common.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/internal/time_keeper.h"
#include "trpc/util/net_util.h"
#include "trpc/util/thread/latch.h"

namespace trpc {

// FrameworkRuntime state manager
RuntimeState framework_runtime_state{RuntimeState::kUnknown};

int RunInTrpcRuntime(std::function<int()>&& func) {
  const GlobalConfig& global_config = TrpcConfig::GetInstance()->GetGlobalConfig();
  const ThreadModelConfig& threadmodel_config = global_config.threadmodel_config;

  // init logging
  TRPC_ASSERT(log::Init());

  if (threadmodel_config.use_fiber_flag) {
    return RunInFiberRuntime(std::move(func));
  }

  return RunInThreadRuntime(std::move(func));
}

int RunInFiberRuntime(std::function<int()>&& func) {
  int status = 0;

  if ((status = InitFrameworkRuntime()) != 0) {
    return status;
  }

  {
    Latch latch(1);
    bool ret = StartFiberDetached([&func, &status, &latch]() {
      TrpcPlugin::GetInstance()->RegisterPlugins();

      try {
        status = func();
      } catch (std::exception& ex) {
        TRPC_FMT_ERROR("exception: {}.", ex.what());
      } catch (...) {
        TRPC_FMT_ERROR("unknown exception.");
      }

      TrpcPlugin::GetInstance()->UnregisterPlugins();

      latch.count_down();
    });

    if (!ret) {
      TRPC_FMT_ERROR("StartFiberDetached failed.");

      latch.count_down();

      status = -1;
    }

    latch.wait();
  }

  DestroyFrameworkRuntime();

  return status;
}

int RunInThreadRuntime(std::function<int()>&& func) {
  const GlobalConfig& global_config = TrpcConfig::GetInstance()->GetGlobalConfig();
  const ThreadModelConfig& threadmodel_config = global_config.threadmodel_config;
  if (threadmodel_config.use_fiber_flag) {
    TRPC_FMT_ERROR("not running in thread runtime, please check the framework config.");
    return -1;
  }

  TrpcPlugin::GetInstance()->RegisterPlugins();

  int status{0};

  try {
    status = func();
  } catch (std::exception& ex) {
    TRPC_FMT_ERROR("exception: {}.", ex.what());
  } catch (...) {
    TRPC_FMT_ERROR("unknown exception.");
  }

  TrpcPlugin::GetInstance()->UnregisterPlugins();

  return status;
}

int InitFrameworkRuntime() {
  // It can be restarted if it has not been started before or has already been destroyed
  if (framework_runtime_state == RuntimeState::kUnknown || framework_runtime_state == RuntimeState::kDestroyed) {
    const GlobalConfig& global_config = TrpcConfig::GetInstance()->GetGlobalConfig();
    const ThreadModelConfig& threadmodel_config = global_config.threadmodel_config;
    if (threadmodel_config.use_fiber_flag) {
      runtime::SetRuntimeType(runtime::kFiberRuntime);
    } else {
      TRPC_FMT_ERROR("not running in fiber runtime, please check the framework config.");
      return -1;
    }

    util::IgnorePipe();

    // set object pool option
    const BufferPoolConfig& buffer_pool_config = global_config.buffer_pool_config;
    memory_pool::SetMemBlockSize(buffer_pool_config.block_size);
    memory_pool::SetMemPoolThreshold(buffer_pool_config.mem_pool_threshold);

    internal::TimeKeeper::Instance()->Start();

    if (IsInFiberRuntime()) {
      fiber::StartRuntime();
      // need to StartAdminRuntime
      separate::StartAdminRuntime();
      runtime::InitFiberReactorConfig();
    }

    framework_runtime_state = RuntimeState::kStarted;
    return 0;
  }

  return -1;
}

bool IsInFiberRuntime() {
  return runtime::IsInFiberRuntime();
}

int DestroyFrameworkRuntime() {
  if (framework_runtime_state != RuntimeState::kStarted) {
    return -1;
  }

  if (IsInFiberRuntime()) {
    separate::TerminateAdminRuntime();

    fiber::TerminateRuntime();
  }

  internal::TimeKeeper::Instance()->Stop();
  internal::TimeKeeper::Instance()->Join();

  TrpcPlugin::GetInstance()->DestroyResource();

  framework_runtime_state = RuntimeState::kDestroyed;

  return 0;
}

}  // namespace trpc
