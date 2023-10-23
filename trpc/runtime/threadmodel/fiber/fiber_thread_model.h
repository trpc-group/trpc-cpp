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

#include <memory>
#include <string>
#include <vector>

#include "trpc/runtime/threadmodel/common/msg_task.h"
#include "trpc/runtime/threadmodel/fiber/detail/fiber_worker.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling_group.h"
#include "trpc/runtime/threadmodel/fiber/detail/timer_worker.h"
#include "trpc/runtime/threadmodel/thread_model.h"
#include "trpc/util/thread/cpu.h"

namespace trpc::fiber {

/// @brief Implementation of fiber m:n coroutine thread model.
class FiberThreadModel final : public ThreadModel {
 public:
  /// @brief Configuration options of thread model instance
  struct Options {
    /// Id of the thread model instance
    uint16_t group_id;

    /// Name of the thread model instance
    std::string group_name;

    /// Name of task scheduler been used
    std::string scheduling_name;

    /// Number of fiber worker threads.
    /// If not specified, it will be set to the number of CPU cores in /proc/cpuinfo
    uint32_t concurrency_hint{0};

    /// Number of fiber worker threads pre sheduling group
    uint32_t scheduling_group_size{0};

    /// Enable numa or not.
    /// If true, the framework will bind the scheduling group to CPU nodes
    /// If false, the framework will not bind the scheduling group to CPU nodes
    bool numa_aware{false};

    /// Specify which CPUs the worker threads should be bound to for execution.
    std::vector<uint32_t> worker_accessible_cpus;

    /// Enable bind core or not
    bool worker_disallow_cpu_migration{false};

    /// Task stealing ratio between different scheduling groups, currently only applicable to the v1 scheduler.
    uint32_t work_stealing_ratio{16};

    /// Task stealing ratio between different scheduling groups in numa architecture, default is no stealing,
    /// currently only applicable to the v1 scheduler.
    uint32_t cross_numa_work_stealing_ratio{0};

    /// The length of the fiber run queue for each scheduling group, it must be a power of 2.
    /// Recommended to be equal or slightly larger than the number of fibers that can be allocated by the system.
    uint32_t run_queue_size{131072};

    /// Fiber stack size. If a large amount of stack resources need to be allocated, adjust this value
    uint32_t stack_size{131072};

    /// The number of pooled fiber memory stacks allocated using mmap to prevent the number of fibers from
    /// exceeding the size of the system's vm.max_map_count and causing fiber creation to fail.
    uint32_t pool_num_by_mmap{30 * 1024};

    /// Enable fiber stack protection or not
    bool stack_enable_guard_page{true};

    /// Does the thread name displayed in the top command use the original process name, default is set by the
    /// framework.
    bool disable_process_name{true};
  };

  // `SchedulingGroup` and its workers (both fiber worker and timer worker).
  struct FullyFledgedSchedulingGroup {
    std::size_t node_id;
    std::unique_ptr<detail::SchedulingGroup> scheduling_group;
    std::vector<std::unique_ptr<detail::FiberWorker>> fiber_workers;
    std::unique_ptr<detail::TimerWorker> timer_worker;
  };

  explicit FiberThreadModel(Options&& options);

  ~FiberThreadModel() override = default;

  std::string_view Type() const override { return kFiber; }

  uint16_t GroupId() const override { return options_.group_id; }

  std::string GroupName() const override { return options_.group_name; }

  void Start() noexcept override;

  void Terminate() noexcept override;

  bool SubmitHandleTask(MsgTask* handle_task) noexcept override;

  detail::SchedulingGroup* NearestSchedulingGroup();

  detail::SchedulingGroup* GetSchedulingGroup(std::size_t index);

  std::size_t GetSchedulingGroupCount() { return flatten_scheduling_groups_.size(); }

  std::size_t GetSchedulingGroupSize() { return options_.scheduling_group_size; }

  std::size_t GetConcurrencyHint() { return options_.concurrency_hint; }

  bool IsNumaAware() { return options_.numa_aware; }

  /// @brief traverse all `SchedulingGroup` to get the size of the fibers to be run in the run queue
  std::size_t GetFiberQueueSize();

  std::vector<FullyFledgedSchedulingGroup*>& GetSchedulingGroups() { return flatten_scheduling_groups_; }

  std::vector<std::unique_ptr<FullyFledgedSchedulingGroup>>& GetSchedulingGroups(std::size_t index) {
    return scheduling_groups_[index];
  }

 private:
  std::unique_ptr<FullyFledgedSchedulingGroup> CreateFullyFledgedSchedulingGroup(uint8_t node_id, uint8_t sg_id,
                                                                                 const std::vector<unsigned>& affinity);

  void InitializeForeignSchedulingGroups(const std::vector<std::unique_ptr<FullyFledgedSchedulingGroup>>& thieves,
                                         const std::vector<std::unique_ptr<FullyFledgedSchedulingGroup>>& victims,
                                         std::uint64_t steal_every_n);
  void InitializeConcurrency();
  void InitializeNumaAwareness();
  void InitializeSchedulingGroupSize();
  void StartWorkersUma(bool is_scheduling_group_size_set);
  void StartWorkersNuma();
  bool IsNumaAwareQualified();
  const std::vector<numa::Node>& GetFiberWorkerAccessibleNodes();
  const std::vector<unsigned>& GetFiberWorkerAccessibleCPUs();
  std::vector<unsigned> GetFiberWorkerAccessibleCPUsImpl();
  std::size_t FindBestSchedulingGroupSize(std::size_t per_node_workers);

 private:
  constexpr static size_t kMaxSchedulingGroupSize = 15;
  constexpr static size_t kMinSchedulingGroupSize = 8;

  Options options_;

  // Index by node ID. i.e., `scheduling_groups_[node][sg_index]`
  //
  // If `numa_aware` is not set, `node` should always be 0.
  //
  // 64 nodes should be enough.
  std::vector<std::unique_ptr<FullyFledgedSchedulingGroup>> scheduling_groups_[64];

  // This vector holds pointer to scheduling groups in `scheduling_groups_`. It's
  // primarily used for randomly choosing a scheduling group or finding scheduling
  // group by ID.
  std::vector<FullyFledgedSchedulingGroup*> flatten_scheduling_groups_;
};

}  // namespace trpc::fiber
