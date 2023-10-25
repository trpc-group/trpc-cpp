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

#include "trpc/runtime/fiber_runtime.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/runtime/threadmodel/fiber/fiber_thread_model.h"
#include "trpc/runtime/threadmodel/thread_model_manager.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/random.h"
#include "trpc/util/thread/thread_helper.h"

namespace trpc::fiber {

// fiber_threadmodel has not owned threadmodel's ownership, it managered by ThreadModelManager
static FiberThreadModel* fiber_threadmodel = nullptr;

namespace detail {

std::size_t GetCurrentSchedulingGroupIndexSlow() {
  auto rc = NearestSchedulingGroupIndex();
  TRPC_CHECK(rc != -1,
             "Calling `GetCurrentSchedulingGroupIndex` outside of any "
             "scheduling group is undefined.");
  return rc;
}

SchedulingGroup* GetSchedulingGroup(std::size_t index) {
  TRPC_ASSERT(fiber_threadmodel != nullptr && "GetSchedulingGroup");
  return fiber_threadmodel->GetSchedulingGroup(index);
}

SchedulingGroup* NearestSchedulingGroup() {
  thread_local detail::SchedulingGroup* nearest{nullptr};
  if (TRPC_LIKELY(nearest)) {
    return nearest;
  }
  if (auto rc = detail::SchedulingGroup::Current()) {
    nearest = rc;
    return rc;
  }

  TRPC_ASSERT(fiber_threadmodel != nullptr && "NearestSchedulingGroup");
  return fiber_threadmodel->NearestSchedulingGroup();
}

std::ptrdiff_t NearestSchedulingGroupIndex() {
  thread_local auto cached = []() -> std::ptrdiff_t {
    auto sg = NearestSchedulingGroup();
    if (sg) {
      TRPC_ASSERT(fiber_threadmodel != nullptr && "NearestSchedulingGroupIndex");
      std::vector<FiberThreadModel::FullyFledgedSchedulingGroup*>& flatten_scheduling_groups =
          fiber_threadmodel->GetSchedulingGroups();
      auto iter = std::find_if(flatten_scheduling_groups.begin(), flatten_scheduling_groups.end(),
                               [&](auto&& e) { return e->scheduling_group.get() == sg; });
      TRPC_CHECK(iter != flatten_scheduling_groups.end());
      return iter - flatten_scheduling_groups.begin();
    }
    return -1;
  }();
  return cached;
}

}  // namespace detail

void StartRuntime() {
  trpc::fiber::FiberThreadModel::Options options;
  options.group_id = ThreadModelManager::GetInstance()->GenGroupId();
  options.group_name = "";
  options.scheduling_name = "v1";

  const GlobalConfig& global_config = TrpcConfig::GetInstance()->GetGlobalConfig();

  // only support one fiber threadmodel instance
  if (global_config.threadmodel_config.fiber_model.size() == 1) {
    const auto& conf = global_config.threadmodel_config.fiber_model[0];
    if (!conf.fiber_scheduling_name.empty()) {
      options.scheduling_name = conf.fiber_scheduling_name;
    }
    options.group_name = conf.instance_name;
    options.concurrency_hint = conf.concurrency_hint;
    options.scheduling_group_size = conf.scheduling_group_size;
    options.numa_aware = conf.numa_aware;

    if (!conf.fiber_worker_accessible_cpus.empty() &&
        ParseBindCoreConfig(conf.fiber_worker_accessible_cpus, options.worker_accessible_cpus)) {
      // Only when configuring the core binding related settings, strict core binding will take effect.
      options.worker_disallow_cpu_migration = conf.fiber_worker_disallow_cpu_migration;
    }
    options.work_stealing_ratio = conf.work_stealing_ratio;
    options.cross_numa_work_stealing_ratio = conf.cross_numa_work_stealing_ratio;
    options.run_queue_size = conf.fiber_run_queue_size;
    options.stack_size = conf.fiber_stack_size;
    options.pool_num_by_mmap = conf.fiber_pool_num_by_mmap;
    options.stack_enable_guard_page = conf.fiber_stack_enable_guard_page;
    options.disable_process_name = global_config.thread_disable_process_name;
    options.enable_gdb_debug = conf.enable_gdb_debug;
  } else {
    options.group_name = "fiber_instance";
  }

  std::shared_ptr<ThreadModel> fiber_model = std::make_shared<FiberThreadModel>(std::move(options));

  fiber_threadmodel = static_cast<FiberThreadModel*>(fiber_model.get());

  TRPC_ASSERT(ThreadModelManager::GetInstance()->Register(fiber_model) == true);

  fiber_threadmodel->Start();
}

void TerminateRuntime() {
  fiber_threadmodel->Terminate();

  ThreadModelManager::GetInstance()->Del(fiber_threadmodel->GroupName());

  fiber_threadmodel = nullptr;
}

ThreadModel* GetFiberThreadModel() { return fiber_threadmodel; }

std::size_t GetSchedulingGroupCount() {
  TRPC_ASSERT(fiber_threadmodel != nullptr);
  return fiber_threadmodel->GetSchedulingGroupCount();
}

std::size_t GetSchedulingGroupSize() {
  TRPC_ASSERT(fiber_threadmodel != nullptr);
  return fiber_threadmodel->GetSchedulingGroupSize();
}

std::size_t GetConcurrencyHint() {
  TRPC_ASSERT(fiber_threadmodel != nullptr);
  return fiber_threadmodel->GetConcurrencyHint();
}

std::size_t GetCurrentSchedulingGroupIndex() {
  thread_local std::size_t index = detail::GetCurrentSchedulingGroupIndexSlow();
  return index;
}

std::size_t GetSchedulingGroupAssignedNode(std::size_t sg_index) {
  TRPC_ASSERT(fiber_threadmodel != nullptr);
  std::vector<FiberThreadModel::FullyFledgedSchedulingGroup*>& flatten_scheduling_groups =
      fiber_threadmodel->GetSchedulingGroups();
  TRPC_CHECK_LT(sg_index, flatten_scheduling_groups.size());
  return flatten_scheduling_groups[sg_index]->node_id;
}

std::size_t GetFiberQueueSize() {
  TRPC_ASSERT(fiber_threadmodel != nullptr);
  return fiber_threadmodel->GetFiberQueueSize();
}

}  // namespace trpc::fiber
