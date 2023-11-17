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
#include <mutex>
#include <string_view>

#include "trpc/runtime/iomodel/reactor/common/epoll_poller.h"
#include "trpc/runtime/iomodel/reactor/common/eventfd_notifier.h"
#include "trpc/runtime/iomodel/reactor/default/timer_queue.h"
#include "trpc/runtime/iomodel/reactor/reactor.h"
#include "trpc/util/align.h"
#include "trpc/util/queue/bounded_mpsc_queue.h"
#ifdef TRPC_BUILD_INCLUDE_ASYNC_IO
#include "trpc/util/async_io/async_io.h"
#endif  // ifdef TRPC_BUILD_INCLUDE_ASYNC_IO

namespace trpc {

/// @brief Reactor implementation on thread
class alignas(hardware_destructive_interference_size) ReactorImpl final : public Reactor {
 public:
  struct Options {
    uint32_t id;

    uint32_t max_task_queue_size{10240};

    bool enable_async_io{false};

    uint32_t io_uring_entries{1024};

    uint32_t io_uring_flags{0};
  };

  explicit ReactorImpl(const Options& options);

  ~ReactorImpl() override = default;

  uint32_t Id() const override { return options_.id; }

  std::string_view Name() const override { return "default_impl"; }

  bool Initialize() override;

  void Run() override;

  void Stop() override;

  void Join() override;

  void Destroy() override;

  void Update(EventHandler* event_handler) override;

  bool SubmitTask(Task&& task, Priority priority) override;

  bool SubmitTask2(Task&& task, Priority priority) override;

  // if submit failed, this function will call `TRPC_ASSERT(false)`
  bool SubmitInnerTask(Task&& task);

  uint64_t AddTimer(TimerTask* timer_task) override;

  uint64_t AddTimerAt(uint64_t expiration, uint64_t interval, TimerExecutor&& executor) override;

  uint64_t AddTimerAfter(uint64_t delay, uint64_t interval, TimerExecutor&& executor) override;

  void PauseTimer(uint64_t timer_id) override;

  void ResumeTimer(uint64_t timer_id) override;

  void CancelTimer(uint64_t timer_id) override;

  void DetachTimer(uint64_t timer_id) override;

  size_t GetTaskSize() override;

#ifdef TRPC_BUILD_INCLUDE_ASYNC_IO
  virtual AsyncIO* GetAsyncIO() const override { return async_io_.get(); }
#endif  // ifdef TRPC_BUILD_INCLUDE_ASYNC_IO

  void ExecuteTask();

 private:
  bool CheckTask();
  bool HandleTask(bool clear);
  bool TrySleep(bool ensure);
  uint64_t GetEpollWaitTimeout();

 private:
  template <typename T>
  using MpScQueue = trpc::BoundedMPSCQueue<T>;

  static constexpr int kContiguousEmptyPolling = 10;
  static constexpr int kMaxTaskCountOnce = 500;

  Options options_;

  std::atomic<bool> stopped_{false};

  std::atomic<bool> is_polling_{false};

  EPollPoller poller_;

  EventFdNotifier task_notifier_;

  EventFdNotifier stop_notifier_;

  TimerQueue timer_queue_;

#ifdef TRPC_BUILD_INCLUDE_ASYNC_IO
  std::unique_ptr<AsyncIO> async_io_;
#endif  // ifdef TRPC_BUILD_INCLUDE_ASYNC_IO

  struct {
    alignas(hardware_destructive_interference_size) MpScQueue<Task> q;
  } task_queues_[kPriorities];

  std::mutex mutex_;
  std::list<Task> inner_task_queue_;
};

}  // namespace trpc
