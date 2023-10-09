//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/fiber_worker.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/threadmodel/fiber/detail/fiber_worker.h"

#include <thread>

#include "trpc/runtime/threadmodel/fiber/detail/fiber_entity.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling_group.h"
#include "trpc/runtime/threadmodel/fiber/detail/stack_allocator_impl.h"
#include "trpc/util/check.h"
#include "trpc/util/thread/latch.h"
#include "trpc/util/thread/thread_helper.h"

namespace trpc::fiber::detail {

FiberWorker::FiberWorker(SchedulingGroup* sg, std::size_t worker_index,
                         bool no_cpu_migration, bool disable_process_name)
    : sg_(sg),
      worker_index_(worker_index),
      no_cpu_migration_(no_cpu_migration),
      disable_process_name_(disable_process_name) {}

void FiberWorker::AddForeignSchedulingGroup(SchedulingGroup* sg, std::uint64_t steal_every_n) {
  sg_->AddForeignSchedulingGroup(worker_index_, sg, steal_every_n);
}

void FiberWorker::Start() noexcept {
  TRPC_CHECK(!no_cpu_migration_ || !sg_->Affinity().empty());
  Latch l(1);
  worker_ = std::thread([this, &l] {
    l.count_down();

    if (!sg_->Affinity().empty()) {
      if (no_cpu_migration_) {
        TRPC_CHECK_LT(worker_index_, sg_->Affinity().size());
        auto cpu = sg_->Affinity()[worker_index_];

        trpc::SetCurrentThreadAffinity({cpu});
        TRPC_FMT_DEBUG("Fiber worker #{} is started on dedicated processsor #{}.", worker_index_, cpu);
      } else {
        trpc::SetCurrentThreadAffinity(sg_->Affinity());
      }
    }
    if (disable_process_name_) {
      std::string index = std::to_string(sg_->GetSchedulingGroupId()) + std::to_string(worker_index_);
      trpc::SetCurrentThreadName("FiberWorker_" + index);
    }

    WorkerThread::SetCurrentWorkerThread(this);
    WorkerProc();
  });

  l.wait();
}

void FiberWorker::Join() noexcept { worker_.join(); }

void FiberWorker::WorkerProc() {
  // Each fiber worker pre-allocates 1024 memory stacks for warming up.
  PrewarmFiberPool(1024);

  sg_->EnterGroup(worker_index_);

  sg_->Schedule();

  TRPC_CHECK_EQ(GetCurrentFiberEntity(), GetMasterFiberEntity());

  sg_->LeaveGroup();
}

}  // namespace trpc::fiber::detail
