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

#include <atomic>
#include <list>
#include <memory>
#include <string_view>

#include "trpc/runtime/iomodel/reactor/common/eventfd_notifier.h"
#include "trpc/runtime/iomodel/reactor/common/epoll_poller.h"
#include "trpc/runtime/iomodel/reactor/reactor.h"
#include "trpc/util/align.h"
#include "trpc/util/queue/bounded_mpsc_queue.h"

namespace trpc {

/// @brief Reactor implementation on fiber
class alignas(hardware_destructive_interference_size) FiberReactor final : public Reactor {
 public:
  struct Options {
    uint32_t id;

    uint32_t max_task_queue_size{65536};
  };

  explicit FiberReactor(const Options& options);

  uint32_t Id() const override { return options_.id; }

  std::string_view Name() const override { return "fiber_impl"; }

  bool Initialize() override;

  void Run() override;

  void Stop() override;

  void Join() override;

  void Destroy() override;

  void Update(EventHandler* event_handler) override;

  bool SubmitTask(Task&& task, Priority priority) override;

  bool SubmitTask2(Task&& task, Priority priority) override;

 private:
  void Dispatch();
  void HandleTask();
  bool CheckTaskQueueSize();

 private:
  template <typename T>
  using MpScQueue = trpc::BoundedMPSCQueue<T>;

  Options options_;

  std::atomic<bool> stopped_{false};

  uint64_t poller_timeout_;

  EPollPoller poller_;

  EventFdNotifier task_notifier_;

  std::mutex mutex_;

  std::list<Task> task_queue_;
};

namespace fiber {

/// @brief Initilize and start running all fiber reactors
void StartAllReactor();

/// @brief Stop and destroy all fiber reactors
void TerminateAllReactor();

/// @brief Get fiber reactor by schedulinggroup and fd
///        `scheduling_group` is used for selecting the scheduling group,
///        `fd` is then used for selecting reactor inside the group,
///        `fd` cannot be 0 or -1, default(-2) means random
Reactor* GetReactor(size_t scheduling_group, int fd = -2);

/// @brief Get all fiber reactor
void GetAllReactor(std::vector<Reactor*>& reactors);

/// @brief Get all fiber reactors under the same scheduling group
void GetReactorInSameGroup(size_t scheduling_group, std::vector<Reactor*>& reactors);

/// @brief Wait until each reactor had executed user's task at least once
void AllReactorsBarrier();

/// @brief Set the number of fiber reactors in the same scheduling group
void SetReactorNumPerSchedulingGroup(size_t size);

/// @brief Set reactor keep running
void SetReactorKeepRunning();

/// @brief Set fiber reactor task queue size
void SetReactorTaskQueueSize(uint32_t size);

}  // namespace fiber

/// @brief Initilize and start running all fiber reactors
void StartAllFiberReactor();

/// @brief Stop all fiber reactors
void StopAllFiberReactor();

/// @brief Wait fiber reactor running completed and destroy all fiber reactors
void JoinAllFiberReactor();

}  // namespace trpc
