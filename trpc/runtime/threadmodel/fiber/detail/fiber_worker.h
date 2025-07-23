//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/fiber_worker.h.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <cstddef>

#include <queue>
#include <thread>

#include "trpc/runtime/threadmodel/fiber/detail/scheduling_group.h"
#include "trpc/runtime/threadmodel/common/worker_thread.h"
#include "trpc/util/align.h"

namespace trpc::fiber::detail {

/// @brief The worker thread that runs the fiber.
class alignas(hardware_destructive_interference_size) FiberWorker : public WorkerThread {
 public:
  /// @brief Constructor a FiberWorker
  /// @param sg The scheduling group to which the worker thread belongs.
  /// @param worker_idex index of worker thread
  FiberWorker(SchedulingGroup* sg, std::size_t worker_index, bool no_cpu_migration = false,
              bool disable_process_name = true);

  inline uint16_t Role() const override { return kIoAndHandle; }

  inline uint16_t Id() const override {
    return static_cast<uint16_t>(sg_->GetSchedulingGroupId() << 8) | worker_index_;
  }

  inline uint16_t GroupId() const override {
    return static_cast<uint16_t>(sg_->GetThreadModelId() << 8) | sg_->GetNodeId();
  }

  std::string GetGroupName() const override { return sg_->GetThreadModeGroupName(); }

  std::string_view GetThreadModelType() const override { return "fiber"; }

  /// @brief Add foreign scheduling group for stealing. May only be called prior to `Start()`.
  /// @param sg foreign scheduling group
  /// @param steal_every_n task stealing factor
  void AddForeignSchedulingGroup(SchedulingGroup* sg, std::uint64_t steal_every_n);

  void Start() noexcept override;

  void Stop() noexcept override {}

  void Join() noexcept override;

  void Destroy() noexcept override {}

  FiberWorker(const FiberWorker&) = delete;
  FiberWorker& operator=(const FiberWorker&) = delete;

 private:
  void WorkerProc();

 private:
  SchedulingGroup* sg_{nullptr};
  std::size_t worker_index_{0};
  bool no_cpu_migration_{false};
  bool disable_process_name_{true};
  std::thread worker_;
};

}  // namespace trpc::fiber::detail
