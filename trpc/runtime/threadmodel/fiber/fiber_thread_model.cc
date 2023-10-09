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

#include "trpc/runtime/threadmodel/fiber/fiber_thread_model.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/runtime/threadmodel/fiber/detail/fiber_entity.h"
#include "trpc/runtime/threadmodel/fiber/detail/fiber_worker.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling/scheduling.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling_group.h"
#include "trpc/runtime/threadmodel/fiber/detail/stack_allocator_impl.h"
#include "trpc/runtime/threadmodel/fiber/detail/timer_worker.h"
#include "trpc/util/deferred.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/object_pool/object_pool.h"
#include "trpc/util/random.h"
#include "trpc/util/string_helper.h"
#include "trpc/util/thread/cpu.h"
#include "trpc/util/thread/thread_helper.h"

namespace trpc::fiber {

namespace {

std::uint64_t DivideRoundUp(std::uint64_t divisor, std::uint64_t dividend) {
  return divisor / dividend + (divisor % dividend != 0);
}

// Call `f` in a thread with the specified affinity.
//
// This method helps you allocates resources from memory attached to one of the
// CPUs listed in `affinity`, instead of the calling node (unless they're the
// same).
template <class F>
void ExecuteWithAffinity(const std::vector<uint32_t>& affinity, F&& f) {
  std::thread([&] {
    trpc::SetCurrentThreadAffinity(affinity);
    std::forward<F>(f)();
  }).join();
}

// For logging purpose only, perf. does not matter.
std::string StringifyCPUs(const std::vector<unsigned>& cpus) {
  std::string result;
  auto cp = cpus;
  std::sort(cp.begin(), cp.end());
  for (auto iter = cp.begin(); iter != cp.end();) {
    auto next = iter;
    while (next + 1 != cp.end()) {
      if (*(next + 1) == *next + 1) {
        ++next;
      } else {
        break;
      }
    }
    if (next == iter + 1) {
      result += std::to_string(*iter) + ",";
    } else {
      result += fmt::format("{}-{},", *iter, *next);
    }
    iter = next + 1;
  }
  TRPC_CHECK(!result.empty());
  result.pop_back();
  return result;
}

}  // namespace

FiberThreadModel::FiberThreadModel(Options&& options) : options_(std::move(options)) {}

void FiberThreadModel::Start() noexcept {
  fiber::detail::SetFiberRunQueueSize(options_.run_queue_size);
  fiber::detail::SetFiberStackSize(options_.stack_size);
  fiber::detail::SetFiberPoolNumByMmap(options_.pool_num_by_mmap);
  fiber::detail::SetFiberStackEnableGuardPage(options_.stack_enable_guard_page);

  InitializeConcurrency();
  InitializeNumaAwareness();

  bool is_scheduling_group_size_setted = options_.scheduling_group_size != 0;
  InitializeSchedulingGroupSize();

  // If CPU migration is explicit disallowed, we need to make sure there are
  // enough CPUs for us.
  auto expected_concurrency =
      DivideRoundUp(options_.concurrency_hint, options_.scheduling_group_size) * options_.scheduling_group_size;
  TRPC_FMT_CRITICAL_IF(
      expected_concurrency > GetFiberWorkerAccessibleCPUs().size() && options_.worker_disallow_cpu_migration,
      "CPU migration of fiber workers is explicitly disallowed, but there "
      "isn't enough CPU to dedicate one for each fiber worker. {} CPUs got, at "
      "least {} CPUs required.",
      GetFiberWorkerAccessibleCPUs().size(), expected_concurrency);

  if (options_.numa_aware) {
    StartWorkersNuma();
  } else {
    StartWorkersUma(is_scheduling_group_size_setted);
  }

  // Fill `flatten_scheduling_groups_`.
  for (auto&& e : scheduling_groups_) {
    for (auto&& ee : e) {
      flatten_scheduling_groups_.push_back(ee.get());
    }
  }

  for (auto&& e : scheduling_groups_) {
    for (auto&& ee : e) {
      ee->timer_worker->Start();
      for (auto&& eee : ee->fiber_workers) {
        eee->Start();
      }
    }
  }
}

void FiberThreadModel::Terminate() noexcept {
  for (auto&& e : scheduling_groups_) {
    for (auto&& ee : e) {
      ee->timer_worker->Stop();
      ee->scheduling_group->Stop();
    }
  }

  for (auto&& e : scheduling_groups_) {
    for (auto&& ee : e) {
      ee->timer_worker->Join();
      for (auto&& eee : ee->fiber_workers) {
        eee->Join();
      }
    }
  }

  for (auto&& e : scheduling_groups_) {
    e.clear();
  }
  flatten_scheduling_groups_.clear();
}

detail::SchedulingGroup* FiberThreadModel::NearestSchedulingGroup() {
  thread_local detail::SchedulingGroup* nearest{nullptr};
  if (TRPC_LIKELY(nearest)) {
    return nearest;
  }
  if (auto rc = detail::SchedulingGroup::Current()) {
    nearest = rc;
    return rc;
  }

  thread_local std::size_t next = trpc::Random();

  auto&& current_node = scheduling_groups_[IsNumaAware() ? numa::GetCurrentNode() : 0];
  if (!current_node.empty()) {
    return current_node[next++ % current_node.size()]->scheduling_group.get();
  }

  if (!flatten_scheduling_groups_.empty()) {
    return flatten_scheduling_groups_[next++ % flatten_scheduling_groups_.size()]->scheduling_group.get();
  }

  return nullptr;
}

detail::SchedulingGroup* FiberThreadModel::GetSchedulingGroup(std::size_t index) {
  TRPC_CHECK_LT(index, flatten_scheduling_groups_.size());
  return flatten_scheduling_groups_[index]->scheduling_group.get();
}

bool FiberThreadModel::SubmitHandleTask(MsgTask* handle_task) noexcept {
  TRPC_ASSERT(handle_task);
  ScopedDeferred _([&] { object_pool::Delete<MsgTask>(handle_task); });

  detail::SchedulingGroup* sg = NearestSchedulingGroup();
  if (handle_task->dst_thread_key >= 0) {
    std::size_t id = handle_task->dst_thread_key % GetSchedulingGroupCount();
    sg = GetSchedulingGroup(id);
  }

  auto desc = fiber::detail::NewFiberDesc();
  if (TRPC_UNLIKELY(desc == nullptr)) {
    TRPC_FMT_ERROR("Create fiber desc failed");
    return false;
  }

  desc->start_proc = std::move(handle_task->handler);
  TRPC_CHECK(!desc->exit_barrier);
  desc->scheduling_group_local = false;

  return sg->StartFiber(desc);
}

std::size_t FiberThreadModel::GetFiberQueueSize() {
  std::size_t sum = std::accumulate(
      flatten_scheduling_groups_.begin(), flatten_scheduling_groups_.end(), 0,
      [](std::size_t a, FullyFledgedSchedulingGroup* b) { return a + b->scheduling_group->GetFiberQueueSize(); });
  return sum;
}

std::unique_ptr<FiberThreadModel::FullyFledgedSchedulingGroup> FiberThreadModel::CreateFullyFledgedSchedulingGroup(
    uint8_t node_id, uint8_t sg_id, const std::vector<unsigned>& affinity) {
  TRPC_CHECK(!options_.worker_disallow_cpu_migration || affinity.size() == options_.scheduling_group_size);
  TRPC_CHECK(options_.scheduling_name == "v1" || options_.scheduling_name == "v2");

  // In order to prevent extra epoll wait when network events are not timely, if only one worker is configured
  // in the scheduling group, two workers will actually be started, one of which is responsible for processing epoll
  // events, and the other handles business logic that requires thread safety.
  // Note: The current logic only applies to the v1 scheduler version.
  std::vector<unsigned> tmp_affinity = affinity;
  uint32_t scheduling_group_size = options_.scheduling_group_size;
  if (options_.scheduling_name == "v1" && options_.scheduling_group_size == 1) {
    scheduling_group_size = 2;
    if (options_.worker_disallow_cpu_migration) {
      TRPC_ASSERT(affinity.size() == 1);
      tmp_affinity.push_back(affinity[0]);
    }
  }

  auto rc = std::make_unique<FullyFledgedSchedulingGroup>();
  rc->node_id = node_id;
  rc->scheduling_group =
      std::make_unique<detail::SchedulingGroup>(tmp_affinity, scheduling_group_size, options_.scheduling_name);
  rc->scheduling_group->SetSchedulingGroupId(sg_id);
  rc->scheduling_group->SetNodeId(node_id);
  rc->scheduling_group->SetThreadModelId(options_.group_id);
  rc->scheduling_group->SetThreadModeGroupName(options_.group_name);

  rc->fiber_workers.reserve(scheduling_group_size);
  for (std::size_t i = 0; i != scheduling_group_size; ++i) {
    rc->fiber_workers.push_back(std::make_unique<detail::FiberWorker>(
        rc->scheduling_group.get(), i, options_.worker_disallow_cpu_migration, options_.disable_process_name));
  }
  rc->timer_worker = std::make_unique<detail::TimerWorker>(rc->scheduling_group.get(), options_.disable_process_name);
  rc->scheduling_group->SetTimerWorker(rc->timer_worker.get());
  return rc;
}

// Add all scheduling groups in `victims` to fiber workers in `thieves`.
//
// Even if scheduling the thief is inside presents in `victims`, it won't be
// added to the corresponding thief.
void FiberThreadModel::InitializeForeignSchedulingGroups(
    const std::vector<std::unique_ptr<FullyFledgedSchedulingGroup>>& thieves,
    const std::vector<std::unique_ptr<FullyFledgedSchedulingGroup>>& victims, std::uint64_t steal_every_n) {
  for (std::size_t thief = 0; thief != thieves.size(); ++thief) {
    for (std::size_t victim = 0; victim != victims.size(); ++victim) {
      if (thieves[thief]->scheduling_group == victims[victim]->scheduling_group) {
        return;
      }
      for (auto&& e : thieves[thief]->fiber_workers) {
        ExecuteWithAffinity(thieves[thief]->scheduling_group->Affinity(), [&] {
          e->AddForeignSchedulingGroup(victims[victim]->scheduling_group.get(), steal_every_n);
        });
      }
    }
  }
}

void FiberThreadModel::StartWorkersUma(bool is_scheduling_group_size_setted) {
  std::uint64_t groups = 1;
  if (is_scheduling_group_size_setted) {
    groups = options_.concurrency_hint < options_.scheduling_group_size
                    ? 1
                    : DivideRoundUp(options_.concurrency_hint, options_.scheduling_group_size);
  }
  TRPC_FMT_DEBUG("Starting {} worker threads per group, for a total of {} groups.", options_.scheduling_group_size,
                 groups);
  TRPC_FMT_WARN_IF(options_.worker_disallow_cpu_migration && GetFiberWorkerAccessibleNodes().size() > 1,
                   "CPU migration of fiber worker is disallowed, and we're trying to start "
                   "in UMA way on NUMA architecture. Performance will likely degrade.");

  for (std::size_t index = 0; index != groups; ++index) {
    if (!options_.worker_disallow_cpu_migration) {
      scheduling_groups_[0].push_back(CreateFullyFledgedSchedulingGroup(0, index, GetFiberWorkerAccessibleCPUs()));
    } else {
      // Each group of processors is dedicated to a scheduling group.
      //
      // Later when we start the fiber workers, we'll instruct them to set their
      // affinity to their dedicated processors.
      auto&& cpus = GetFiberWorkerAccessibleCPUs();
      TRPC_CHECK_LE((index + 1) * options_.scheduling_group_size, cpus.size());
      scheduling_groups_[0].push_back(
          CreateFullyFledgedSchedulingGroup(0, index,
                                            {cpus.begin() + index * options_.scheduling_group_size,
                                             cpus.begin() + (index + 1) * options_.scheduling_group_size}));
    }
  }

  InitializeForeignSchedulingGroups(scheduling_groups_[0], scheduling_groups_[0], options_.work_stealing_ratio);
}

void FiberThreadModel::StartWorkersNuma() {
  auto topo = GetFiberWorkerAccessibleNodes();
  TRPC_CHECK_LT(topo.size(), std::size(scheduling_groups_),
                "Far more nodes that Trpc-Cpp can support present on this "
                "machine. Bail out.");

  auto expected_groups = DivideRoundUp(options_.concurrency_hint, options_.scheduling_group_size);
  auto groups_per_node = DivideRoundUp(expected_groups, topo.size());
  TRPC_FMT_DEBUG(
      "Starting {} worker threads per group, {} group per node, for a total of "
      "{} nodes.",
      options_.scheduling_group_size, groups_per_node, topo.size());

  for (std::size_t i = 0; i != topo.size(); ++i) {
    for (std::size_t j = 0; j != groups_per_node; ++j) {
      if (!options_.worker_disallow_cpu_migration) {
        auto&& affinity = topo[i].logical_cpus;
        ExecuteWithAffinity(
            affinity, [&] { scheduling_groups_[i].push_back(CreateFullyFledgedSchedulingGroup(i, j, affinity)); });
      } else {
        // Same as `StartWorkersUma()`, fiber worker's affinity is set upon
        // start.
        auto&& cpus = topo[i].logical_cpus;
        TRPC_CHECK_LE((j + 1) * options_.scheduling_group_size, cpus.size());
        std::vector<unsigned> affinity = {cpus.begin() + j * options_.scheduling_group_size,
                                          cpus.begin() + (j + 1) * options_.scheduling_group_size};
        ExecuteWithAffinity(
            affinity, [&] { scheduling_groups_[i].push_back(CreateFullyFledgedSchedulingGroup(i, j, affinity)); });
      }
    }
  }

  for (std::size_t i = 0; i != topo.size(); ++i) {
    for (std::size_t j = 0; j != topo.size(); ++j) {
      if (options_.cross_numa_work_stealing_ratio == 0 && i != j) {
        // Different NUMA domain.
        //
        // `trpc_enable_cross_numa_work_stealing` is not enabled, so we skip
        // this pair.
        continue;
      }
      InitializeForeignSchedulingGroups(
          scheduling_groups_[i], scheduling_groups_[j],
          i == j ? options_.work_stealing_ratio : options_.cross_numa_work_stealing_ratio);
    }
  }
}

const std::vector<numa::Node>& FiberThreadModel::GetFiberWorkerAccessibleNodes() {
  static std::vector<numa::Node> result = [this] {
    std::map<unsigned, std::vector<unsigned>> node_to_processor;
    for (auto&& e : this->GetFiberWorkerAccessibleCPUs()) {
      auto n = numa::GetNodeOfProcessor(e);
      node_to_processor[n].push_back(e);
    }

    std::vector<numa::Node> result;
    for (auto&& [k, v] : node_to_processor) {
      result.push_back({k, v});
    }
    return result;
  }();
  return result;
}

void FiberThreadModel::InitializeConcurrency() {
  TRPC_FMT_DEBUG("Accessible CPUs: {}", StringifyCPUs(GetFiberWorkerAccessibleCPUs()));

  if (options_.concurrency_hint == 0) {
    options_.concurrency_hint = GetFiberWorkerAccessibleCPUs().size();
  }
}

const std::vector<unsigned>& FiberThreadModel::GetFiberWorkerAccessibleCPUs() {
  static auto result = GetFiberWorkerAccessibleCPUsImpl();
  return result;
}

std::vector<unsigned> FiberThreadModel::GetFiberWorkerAccessibleCPUsImpl() {
  if (!options_.worker_accessible_cpus.empty()) {
    return options_.worker_accessible_cpus;
  }

  auto affinity = GetCurrentThreadAffinity();
  if (affinity.size() && affinity.size() != GetNumberOfProcessorsConfigured()) {
    return affinity;
  }

  std::vector<unsigned> result;
  for (std::size_t i = 0; i != GetNumberOfProcessorsConfigured(); ++i) {
    if (IsProcessorAccessible(i)) {
      result.push_back(i);
    } else {  // Containerized environment otherwise?
      TRPC_FMT_DEBUG("Processor #{} is not accessible to us, ignored.", i);
    }
  }
  return result;
}

void FiberThreadModel::InitializeNumaAwareness() {
  options_.numa_aware = IsNumaAwareQualified();

  TRPC_FMT_DEBUG("NUMA-awareness is {}.", options_.numa_aware ? "enabled" : "not enabled");
}

bool FiberThreadModel::IsNumaAwareQualified() {
  // If user explicitly specified the parameter, respect user's choice.
  if (options_.numa_aware) {
    return true;
  }

  // If only 1 node is accessible to us, there's no NUMA to deal with anyway.
  if (GetFiberWorkerAccessibleNodes().size() == 1) {
    TRPC_LOG_DEBUG(
        "UMA environment (in terms of accessible processors to us) detected, "
        "disabling NUMA-awareness.");
    return false;
  }

  // If not all processor are accessible to us, and they're not spread between
  // NUMA domains evenly, do not enable NUMA awareness.
  if (GetFiberWorkerAccessibleCPUs().size() != GetNumberOfProcessorsConfigured()) {
    std::map<int, std::size_t> processors_in_node;
    for (auto&& e : GetFiberWorkerAccessibleCPUs()) {
      ++processors_in_node[numa::GetNodeOfProcessor(e)];
    }
    auto it = processors_in_node.begin();
    while (it != processors_in_node.end()) {
      if (it->second != processors_in_node.begin()->second) {
        TRPC_LOG_DEBUG(
            "Not all CPUs are accessible to us, and the accessible ones are "
            "not spread between NUMA nodes evenly. Disabling NUMA-awareness.");
        return false;
      }
      ++it;
    }
    // Fallthrough otherwise.
  }

  if (options_.concurrency_hint < GetFiberWorkerAccessibleNodes().size()) {
    TRPC_LOG_DEBUG(
        "There are more NUMA nodes than concurrency requested, disabling "
        "NUMA-awareness.");
    return false;
  }

  if (options_.concurrency_hint > 0 && options_.concurrency_hint <= kMaxSchedulingGroupSize) {
    TRPC_LOG_DEBUG(
        "The requested concurrency is too low to enable NUMA-awareness, "
        "disabling it.");
    return false;
  }

  if (options_.concurrency_hint > 0 && options_.scheduling_group_size > 0 &&
      (DivideRoundUp(options_.concurrency_hint, options_.scheduling_group_size) %
       GetFiberWorkerAccessibleNodes().size()) != 0) {
    TRPC_LOG_DEBUG(
        "Both `trpc_concurrency_hint` and `trpc_scheduling_group_size` are "
        "set, and the resulting group count is not a multiple of (nor equal "
        "to) the number of NUMA nodes, disabling NUMA-awareness.");
    return false;
  }

  return true;
}

void FiberThreadModel::InitializeSchedulingGroupSize() {
  if (options_.scheduling_group_size == 0) {
    auto per_node_workers = options_.concurrency_hint;
    if (options_.numa_aware) {
      per_node_workers = DivideRoundUp(options_.concurrency_hint, GetFiberWorkerAccessibleNodes().size());
    }
    options_.scheduling_group_size = FindBestSchedulingGroupSize(per_node_workers);
    options_.scheduling_group_size = std::min(options_.scheduling_group_size, options_.concurrency_hint);
  }
}

std::size_t FiberThreadModel::FindBestSchedulingGroupSize(std::size_t per_node_workers) {
  if (per_node_workers <= kMaxSchedulingGroupSize) {
    return std::max(per_node_workers, kMinSchedulingGroupSize);
  }
  std::size_t best_i = kMaxSchedulingGroupSize;
  std::size_t min_distance = kMaxSchedulingGroupSize;
  // Find i that an integer multiple of i is bigger than per_node_workers
  // and closest to per_node_workers.
  for (size_t i = kMaxSchedulingGroupSize; i >= kMinSchedulingGroupSize; --i) {
    auto remainder = per_node_workers % i;
    if (remainder == 0) {
      return i;
    }
    if (auto distance = i - remainder; distance < min_distance) {
      min_distance = distance;
      best_i = i;
    }
  }
  return best_i;
}

}  // namespace trpc::fiber
