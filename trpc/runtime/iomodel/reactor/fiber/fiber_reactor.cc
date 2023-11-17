//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/io/event_loop.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/iomodel/reactor/fiber/fiber_reactor.h"

#include <utility>

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/fiber_local.h"
#include "trpc/runtime/fiber_runtime.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/random.h"

namespace trpc {

struct FiberReactorWorker {
  std::unique_ptr<Reactor> reactor;
  Fiber fiber;
};

std::vector<std::vector<FiberReactorWorker>> fiber_reactor_workers;
uint32_t reactor_num_per_scheduling_group = 1;
bool reactor_keep_running = false;
uint32_t reactor_task_queue_size = 65536;

FiberReactor::FiberReactor(const Options& options)
    : options_(options),
      task_notifier_(this) {
}

bool FiberReactor::Initialize() {
  task_notifier_.EnableNotify();

  return true;
}

void FiberReactor::Stop() { stopped_.store(true, std::memory_order_relaxed); }

void FiberReactor::Join() {}

void FiberReactor::Destroy() {
  task_notifier_.DisableNotify();
}

void FiberReactor::Run() {
  SetCurrentTlsReactor(nullptr);

  while (!stopped_.load(std::memory_order_relaxed)) {
    Dispatch();

    HandleTask();

    if (!reactor_keep_running) {
      if (!CheckTaskQueueSize()) {
        FiberYield();
      } else {
        HandleTask();
      }
    }
  }

  HandleTask();
}

void FiberReactor::Update(EventHandler* event_handler) {
  poller_.UpdateEvent(event_handler);
}

bool FiberReactor::SubmitTask(Task&& task, [[maybe_unused]] Priority priority) {
  {
    std::scoped_lock lk(mutex_);
    task_queue_.push_back(std::move(task));
  }

  task_notifier_.WakeUp();

  return true;
}

bool FiberReactor::SubmitTask2(Task&& task, Priority priority) {
  return SubmitTask(std::move(task), priority);
}

bool FiberReactor::CheckTaskQueueSize() {
  std::scoped_lock _(mutex_);
  return task_queue_.size() > 0;
}

void FiberReactor::Dispatch() {
  poller_.Dispatch(Poller::kPollerTimeout);
}

void FiberReactor::HandleTask() {
  std::list<Task> task_queue;
  {
    std::scoped_lock lk(mutex_);
    task_queue.swap(task_queue_);
  }

  while (!task_queue.empty()) {
    task_queue.front()();
    task_queue.pop_front();
  }
}

void StartAllFiberReactor() {
  fiber::StartAllReactor();
}

void StopAllFiberReactor() {
  for (auto&& elws : fiber_reactor_workers) {
    for (auto&& rtw : elws) {
      rtw.reactor->Stop();
    }
  }
}

void JoinAllFiberReactor() {
  for (auto&& elws : fiber_reactor_workers) {
    for (auto&& rtw : elws) {
      rtw.reactor->Join();
    }
  }

  for (auto&& elws : fiber_reactor_workers) {
    for (auto&& rtw : elws) {
      rtw.fiber.Join();
    }
  }

  for (auto&& elws : fiber_reactor_workers) {
    for (auto&& rtw : elws) {
      rtw.reactor->Destroy();
    }
  }

  fiber_reactor_workers.clear();
}

namespace fiber {

void SetReactorNumPerSchedulingGroup(size_t size) {
  reactor_num_per_scheduling_group = size;
}

void SetReactorKeepRunning() {
  reactor_keep_running = true;
}

void SetReactorTaskQueueSize(uint32_t size) {
  reactor_task_queue_size = size;
}

uint32_t HashFd(int fd) {
  auto xorshift = [](std::uint64_t n, std::uint64_t i) { return n ^ (n >> i); };
  uint64_t p = 0x5555555555555555;
  uint64_t c = 17316035218449499591ull;
  return c * xorshift(p * xorshift(fd, 32), 32);
}

void StartAllReactor() {
  FiberLatch all_started(fiber::GetSchedulingGroupCount() * reactor_num_per_scheduling_group);

  fiber_reactor_workers.resize(fiber::GetSchedulingGroupCount());
  for (std::size_t sgi = 0; sgi != fiber_reactor_workers.size(); ++sgi) {
    fiber_reactor_workers[sgi].resize(reactor_num_per_scheduling_group);
    for (std::size_t rti = 0; rti != reactor_num_per_scheduling_group; ++rti) {
      auto&& rtw = fiber_reactor_workers[sgi][rti];
      auto start_cb = [&all_started, ertw = &rtw] {
        TRPC_ASSERT(ertw->reactor->SubmitTask([&all_started] { all_started.CountDown(); }));
        ertw->reactor->Run();
      };

      FiberReactor::Options options;
      options.id = (static_cast<uint32_t>(sgi) << 16) | rti;
      options.max_task_queue_size = reactor_task_queue_size;

      rtw.reactor = std::make_unique<FiberReactor>(options);
      TRPC_ASSERT(rtw.reactor->Initialize());

      rtw.fiber = Fiber(Fiber::Attributes{.launch_policy = fiber::Launch::Post,
                                          .scheduling_group = sgi,
                                          .execution_context = nullptr,
                                          .scheduling_group_local = true,
                                          .is_fiber_reactor = true},
                        start_cb);
    }
  }
  all_started.Wait();
}

void TerminateAllReactor() {
  for (auto&& elws : fiber_reactor_workers) {
    for (auto&& rtw : elws) {
      rtw.reactor->Stop();
    }
  }

  for (auto&& elws : fiber_reactor_workers) {
    for (auto&& rtw : elws) {
      rtw.reactor->Join();
    }
  }

  for (auto&& elws : fiber_reactor_workers) {
    for (auto&& rtw : elws) {
      rtw.fiber.Join();
    }
  }

  for (auto&& elws : fiber_reactor_workers) {
    for (auto&& rtw : elws) {
      rtw.reactor->Destroy();
    }
  }

  fiber_reactor_workers.clear();
}

Reactor* GetReactor(std::size_t scheduling_group, int fd) {
  TRPC_CHECK(fd != 0 && fd != -1,
              "You're likely passing in a fd got from calling `Get()` on an "
              "invalid `Handle`.");
  if (fd == -2) {
    fd = Random<int>();
  }

  TRPC_ASSERT(scheduling_group < fiber_reactor_workers.size());

  auto rti = HashFd(fd) % reactor_num_per_scheduling_group;
  auto&& ptr = fiber_reactor_workers[scheduling_group][rti].reactor;
  TRPC_ASSERT(!!ptr);
  return ptr.get();
}

void GetAllReactor(std::vector<Reactor*>& reactors) {
  reactors.clear();
  for (std::size_t sgi = 0; sgi != fiber_reactor_workers.size(); ++sgi) {
    for (std::size_t rti = 0; rti != reactor_num_per_scheduling_group; ++rti) {
      TRPC_ASSERT(fiber_reactor_workers[sgi][rti].reactor.get());
      reactors.push_back(fiber_reactor_workers[sgi][rti].reactor.get());
    }
  }
}

void GetReactorInSameGroup(size_t scheduling_group, std::vector<Reactor*>& reactors) {
  TRPC_ASSERT(scheduling_group < fiber_reactor_workers.size());

  reactors.clear();
  for (std::size_t rti = 0; rti != reactor_num_per_scheduling_group; ++rti) {
    TRPC_ASSERT(fiber_reactor_workers[scheduling_group][rti].reactor.get());
    reactors.push_back(fiber_reactor_workers[scheduling_group][rti].reactor.get());
  }
}

void AllReactorsBarrier() {
  FiberLatch l(fiber_reactor_workers.size() * reactor_num_per_scheduling_group);
  for (auto&& elws : fiber_reactor_workers) {
    for (auto&& rtw : elws) {
      rtw.reactor->SubmitTask([&] { l.CountDown(); });
    }
  }
  l.Wait();
}

}  // namespace fiber

}  // namespace trpc
