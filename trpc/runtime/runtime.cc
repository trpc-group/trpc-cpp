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

#include "trpc/runtime/runtime.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"
#include "trpc/runtime/fiber_runtime.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_reactor.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/separate_runtime.h"
#include "trpc/runtime/threadmodel/thread_model_manager.h"
#include "trpc/util/internal/time_keeper.h"
#include "trpc/util/buffer/memory_pool/common.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/latch.h"
#include "trpc/util/net_util.h"

namespace trpc::runtime {

namespace {

void PrewarmObjectPools() {
  constexpr auto kFibersPerSchedulingGroup = 1024;
  constexpr auto kBufferSizePerFiber = 131072;

  for (std::size_t i = 0; i != fiber::GetSchedulingGroupCount(); ++i) {
    for (int j = 0; j != kFibersPerSchedulingGroup; ++j) {
      // Pre-allocate some fiber stacks for use.
      Fiber::Attributes attr;
      attr.scheduling_group = i;

      bool start_fiber = StartFiberDetached(std::move(attr), [] {
        // Warms object pool used by `NoncontiguousBuffer`.
        static char temp[kBufferSizePerFiber];
        [[maybe_unused]] NoncontiguousBufferBuilder builder;
        builder.Append(temp, sizeof(temp));
      });
      if (!start_fiber) {
        TRPC_ASSERT(false && "StartFiberDetached to execute PrewarmObjectPools faild, abort!!!");
      }
    }
  }
}

}  // namespace

static uint32_t global_runtime_type = kThreadRuntime;

uint32_t GetRuntimeType() {
  return global_runtime_type;
}

void SetRuntimeType(uint32_t type) {
  global_runtime_type = type;
}

bool IsInFiberRuntime() {
  return global_runtime_type == kFiberRuntime;
}

bool StartRuntime() {
  const GlobalConfig& global_config = TrpcConfig::GetInstance()->GetGlobalConfig();
  const ThreadModelConfig& threadmodel_config = global_config.threadmodel_config;
  if (threadmodel_config.use_fiber_flag) {
    SetRuntimeType(kFiberRuntime);
  }

  util::IgnorePipe();

  // set object pool option
  const BufferPoolConfig& buffer_pool_config = global_config.buffer_pool_config;
  memory_pool::SetMemBlockSize(buffer_pool_config.block_size);
  memory_pool::SetMemPoolThreshold(buffer_pool_config.mem_pool_threshold);

  internal::TimeKeeper::Instance()->Start();

  if (IsInFiberRuntime()) {
    fiber::StartRuntime();

    InitFiberReactorConfig();
  }

  merge::StartRuntime();
  separate::StartRuntime();
  separate::StartAdminRuntime();

  return true;
}

bool Run(std::function<void()>&& func) {
  TRPC_ASSERT(func);

  if (IsInFiberRuntime()) {
    Latch l(1);
    bool start_fiber_runtime = StartFiberDetached([&] {
      PrewarmObjectPools();

      fiber::StartAllReactor();

      func();

      fiber::TerminateAllReactor();

      l.count_down();
    });

    if (start_fiber_runtime) {
      l.wait();
    } else {
      // print err and return
      std::cerr << "StartFiberDetached failed." << std::endl;
      return false;
    }
  } else {
    func();
  }

  return true;
}

void TerminateRuntime() {
  separate::TerminateAdminRuntime();
  separate::TerminateRuntime();
  merge::TerminateRuntime();

  if (IsInFiberRuntime()) {
    fiber::TerminateRuntime();
  }

  internal::TimeKeeper::Instance()->Stop();
  internal::TimeKeeper::Instance()->Join();
}

void InitFiberReactorConfig() {
  const GlobalConfig& global_config = TrpcConfig::GetInstance()->GetGlobalConfig();
  TRPC_ASSERT(global_config.threadmodel_config.use_fiber_flag);

  const auto& conf = global_config.threadmodel_config.fiber_model[0];
  size_t reactor_num_per_scheduling_group = 1;

  // note: GetSchedulingGroupSize valid only after fiber StartRuntime has executed
  if (conf.reactor_num_per_scheduling_group > 0) {
    // if set `reactor_num_per_scheduling_group`
    // it should not exceed the value of GetSchedulingGroupSize/2
    reactor_num_per_scheduling_group =
        std::min(conf.reactor_num_per_scheduling_group, (uint32_t)fiber::GetSchedulingGroupSize() / 2);
  }

  if (reactor_num_per_scheduling_group <= 0) {
    // if not set `reactor_num_per_scheduling_group` or `reactor_num_per_scheduling_group` invalid
    // set 1
    reactor_num_per_scheduling_group = 1;
  }

  fiber::SetReactorNumPerSchedulingGroup(reactor_num_per_scheduling_group);

  // In order to prevent extra epoll wait when network events are not timely, if only one worker is configured
  // in the scheduling group, two workers will actually be started, one of which is responsible for processing epoll
  // events, and the other handles business logic that requires thread safety.
  // Note: The current logic only applies to the v1 scheduler version.
  bool is_v1_scheduler = (conf.fiber_scheduling_name.empty() || conf.fiber_scheduling_name == "v1");
  if (is_v1_scheduler && fiber::GetSchedulingGroupSize() == 1) {
    fiber::SetReactorKeepRunning();
  }

  fiber::SetReactorTaskQueueSize(conf.reactor_task_queue_size);
}

}  // namespace trpc::runtime
