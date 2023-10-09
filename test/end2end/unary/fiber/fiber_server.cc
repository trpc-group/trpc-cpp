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

#include "trpc/common/future/future_utility.h"
#include "trpc/coroutine/async.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_blocking_bounded_queue.h"
#include "trpc/coroutine/fiber_blocking_noncontiguous_buffer.h"
#include "trpc/coroutine/fiber_condition_variable.h"
#include "trpc/coroutine/fiber_event.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/fiber_local.h"
#include "trpc/coroutine/fiber_seqlock.h"
#include "trpc/coroutine/fiber_shared_mutex.h"
#include "trpc/coroutine/fiber_timer.h"
#include "trpc/coroutine/future.h"
#include "trpc/runtime/fiber_runtime.h"

#include "test/end2end/unary/fiber/fiber.trpc.pb.h"
#include "test/end2end/unary/fiber/fiber_server.h"

namespace trpc::testing {

class FiberServerImpl : public trpc::testing::fibersvr::FiberService {
  ::trpc::Status TestFiber(::trpc::ServerContextPtr context, const ::trpc::testing::fibersvr::TestRequest* request,
                           ::trpc::testing::fibersvr::TestReply* response) {
    if (request->msg() == "default") {
      int count = 0;
      ::trpc::Fiber fiber = ::trpc::Fiber([&count] { ++count; });

      fiber.Join();

      response->set_msg(std::to_string(count));
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "dispatch") {
      bool is_current_thread = false;
      auto before_thread_id = std::this_thread::get_id();
      Fiber::Attributes attr;
      // Execute immediately on the current thread without initiating a new scheduling
      attr.launch_policy = fiber::Launch::Dispatch;

      ::trpc::Fiber fiber = ::trpc::Fiber(attr, [&] {
        if (before_thread_id == std::this_thread::get_id()) {
          is_current_thread = true;
        }
      });

      fiber.Join();

      response->set_msg(std::to_string(is_current_thread));
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "scheduling_group") {
      bool is_current_scheduling_group = false;
      std::size_t before_scheduling_group_index = ::trpc::fiber::GetCurrentSchedulingGroupIndex();

      Fiber::Attributes attr;
      // Set to another scheduling group
      attr.scheduling_group = 1;

      ::trpc::Fiber fiber = ::trpc::Fiber(attr, [&] {
        if (before_scheduling_group_index != ::trpc::fiber::GetCurrentSchedulingGroupIndex()) {
          is_current_scheduling_group = false;
        }
      });

      fiber.Join();

      response->set_msg(std::to_string(is_current_scheduling_group));
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "start_fiber_detached_default") {
      trpc::FiberLatch l(1);
      int count = 0;
      trpc::StartFiberDetached([&l, &count] {
        count++;

        l.CountDown();
      });

      l.Wait();

      response->set_msg(std::to_string(count));
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "start_fiber_detached_dispatch") {
      trpc::FiberLatch l(1);
      bool is_current_thread = false;
      auto before_thread_id = std::this_thread::get_id();

      Fiber::Attributes attr;
      // Execute immediately on the current thread without initiating a new scheduling
      attr.launch_policy = fiber::Launch::Dispatch;
      trpc::StartFiberDetached([&l, &before_thread_id, &is_current_thread] {
        if (before_thread_id == std::this_thread::get_id()) {
          is_current_thread = true;
        }

        l.CountDown();
      });

      l.Wait();

      response->set_msg(std::to_string(is_current_thread));
      return ::trpc::kSuccStatus;
    }

    return ::trpc::kUnknownErrorStatus;
  }

  ::trpc::Status TestFiberMutex(::trpc::ServerContextPtr context, const ::trpc::testing::fibersvr::TestRequest* request,
                                ::trpc::testing::fibersvr::TestReply* response) {
    if (request->msg() == "normal_case") {
      int loop_count = 8;
      ::trpc::FiberLatch l(loop_count);
      ::trpc::FiberMutex m;
      int value = 0;
      for (int i = 0; i != loop_count; ++i) {
        ::trpc::StartFiberDetached([&l, &m, &value] {
          // If we don't lock here, the value cannot be guaranteed to be B
          std::scoped_lock _(m);
          ++value;

          l.CountDown();
        });
      }

      l.Wait();

      response->set_msg(std::to_string(value));
      return ::trpc::kSuccStatus;
    }

    return ::trpc::kUnknownErrorStatus;
  }

  ::trpc::Status TestFiberConditionVariable(::trpc::ServerContextPtr context,
                                            const ::trpc::testing::fibersvr::TestRequest* request,
                                            ::trpc::testing::fibersvr::TestReply* response) {
    if (request->msg() == "wait") {
      // FiberConditionVariable is generally used in combination with FiberMutex
      std::atomic<std::size_t> run = 0;
      ::trpc::FiberMutex fiber_lock;
      ::trpc::FiberConditionVariable cv;

      ::trpc::Fiber fiber1 = ::trpc::Fiber([&run, &cv, &fiber_lock] {
        std::unique_lock lk(fiber_lock);
        cv.wait(lk);
        ++run;
      });

      ::trpc::Fiber fiber2 = ::trpc::Fiber([&cv, &fiber_lock] {
        ::trpc::FiberSleepFor(std::chrono::milliseconds(100));
        std::scoped_lock _(fiber_lock);
        cv.notify_one();
      });

      fiber1.Join();
      fiber2.Join();

      response->set_msg(std::to_string(run));
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "wait_predicate") {
      // FiberConditionVariable is generally used in combination with FiberMutex
      std::atomic<std::size_t> run = 0;
      bool predicate = false;
      ::trpc::FiberMutex fiber_lock;
      ::trpc::FiberConditionVariable cv;

      ::trpc::Fiber fiber1 = ::trpc::Fiber([&run, &cv, &fiber_lock, &predicate] {
        std::unique_lock lk(fiber_lock);
        // Waiting for the condition to become true
        cv.wait(lk, [&] { return predicate; });

        ++run;
      });

      ::trpc::Fiber fiber2 = ::trpc::Fiber([&cv, &fiber_lock, &predicate] {
        ::trpc::FiberSleepFor(std::chrono::milliseconds(100));
        std::scoped_lock _(fiber_lock);
        predicate = true;
        cv.notify_one();
      });

      fiber1.Join();
      fiber2.Join();

      response->set_msg(std::to_string(run));
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "wait_for") {
      // FiberConditionVariable is generally used in combination with FiberMutex
      std::atomic<std::size_t> run = 0;
      ::trpc::FiberMutex fiber_lock;
      ::trpc::FiberConditionVariable cv;

      ::trpc::Fiber fiber1 = ::trpc::Fiber([&run, &cv, &fiber_lock] {
        std::unique_lock lk(fiber_lock);
        // Waiting to be awakened or timed out
        cv.wait_for(lk, std::chrono::milliseconds(100));
        ++run;
      });

      ::trpc::Fiber fiber2 = ::trpc::Fiber([&cv, &fiber_lock] {
        // Here, sleep is used to simulate not being awakened by notify within the timeout period of the condition
        // variable
        ::trpc::FiberSleepFor(std::chrono::milliseconds(200));
        std::unique_lock lk(fiber_lock);
        cv.notify_one();
      });

      fiber1.Join();
      fiber2.Join();

      response->set_msg(std::to_string(run));
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "wait_for_predicate") {
      // FiberConditionVariable is generally used in combination with FiberMutex
      std::atomic<std::size_t> run = 0;
      bool predicate = false;
      ::trpc::FiberMutex fiber_lock;
      ::trpc::FiberConditionVariable cv;

      ::trpc::Fiber fiber1 = ::trpc::Fiber([&run, &cv, &fiber_lock, &predicate] {
        std::unique_lock lk(fiber_lock);
        // Waiting for the condition to become true or timeout
        cv.wait_for(lk, std::chrono::milliseconds(100), [&] { return predicate; });
        ++run;
      });

      ::trpc::Fiber fiber2 = ::trpc::Fiber([&cv, &fiber_lock, &predicate] {
        // Here, sleep is used to simulate not being awakened by notify within the timeout period of the condition
        // variable
        ::trpc::FiberSleepFor(std::chrono::milliseconds(200));
        std::scoped_lock _(fiber_lock);
        predicate = true;
        cv.notify_one();
      });

      fiber1.Join();
      fiber2.Join();

      response->set_msg(std::to_string(run));
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "wait_until") {
      // FiberConditionVariable is generally used in combination with FiberMutex
      std::atomic<std::size_t> run = 0;
      ::trpc::FiberMutex fiber_lock;
      ::trpc::FiberConditionVariable cv;

      ::trpc::Fiber fiber1 = ::trpc::Fiber([&run, &cv, &fiber_lock] {
        std::unique_lock lk(fiber_lock);
        // Waiting for the condition to become true or timeout
        cv.wait_until(lk, ::trpc::ReadSteadyClock() + std::chrono::milliseconds(100));

        ++run;
      });

      ::trpc::Fiber fiber2 = ::trpc::Fiber([&cv, &fiber_lock] {
        // Here, sleep is used to simulate not being awakened by notify within the timeout period of the condition
        // variable.
        ::trpc::FiberSleepFor(std::chrono::milliseconds(200));
        std::unique_lock lk(fiber_lock);
        cv.notify_one();
      });

      fiber1.Join();
      fiber2.Join();

      response->set_msg(std::to_string(run));
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "wait_until_predicate") {
      // FiberConditionVariable is generally used in combination with FiberMutex
      std::atomic<std::size_t> run = 0;
      bool predicate = false;
      ::trpc::FiberMutex fiber_lock;
      ::trpc::FiberConditionVariable cv;

      ::trpc::Fiber fiber1 = ::trpc::Fiber([&run, &cv, &fiber_lock, &predicate] {
        std::unique_lock lk(fiber_lock);
        // Waiting for the condition to become true or timeout
        cv.wait_until(lk, ::trpc::ReadSteadyClock() + std::chrono::milliseconds(100), [&] { return predicate; });

        ++run;
      });

      ::trpc::Fiber fiber2 = ::trpc::Fiber([&cv, &fiber_lock, &predicate] {
        // Here, sleep is used to simulate not being awakened by notify within the timeout period of the condition
        // variable.
        ::trpc::FiberSleepFor(std::chrono::milliseconds(200));
        std::scoped_lock _(fiber_lock);
        predicate = true;
        cv.notify_one();
      });

      fiber1.Join();
      fiber2.Join();

      response->set_msg(std::to_string(run));
      return ::trpc::kSuccStatus;
    }

    return ::trpc::kUnknownErrorStatus;
  }

  ::trpc::Status TestFiberLatch(::trpc::ServerContextPtr context, const ::trpc::testing::fibersvr::TestRequest* request,
                                ::trpc::testing::fibersvr::TestReply* response) {
    if (request->msg() == "wait") {
      std::atomic<std::size_t> run = 0;
      ::trpc::FiberLatch latch(1);

      // Returns true if the count is greater than 0.
      latch.TryWait();
      ::trpc::StartFiberDetached([&run, &latch] {
        latch.CountDown();

        // Returns false if the count is equal to 0
        latch.TryWait();

        ++run;
      });

      latch.Wait();

      response->set_msg(std::to_string(run));
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "wait_for") {
      bool wait_result = false;
      ::trpc::FiberLatch latch(1);

      // Wait until count equals 0 or timeout
      wait_result = latch.WaitFor(std::chrono::milliseconds(100));

      response->set_msg(std::to_string(wait_result));
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "wait_until") {
      bool wait_result = false;
      ::trpc::FiberLatch latch(1);

      // Wait until count equals 0 or timeout
      wait_result = latch.WaitUntil(::trpc::ReadSteadyClock() + std::chrono::milliseconds(100));

      response->set_msg(std::to_string(wait_result));
      return ::trpc::kSuccStatus;
    }

    return ::trpc::kUnknownErrorStatus;
  }

  ::trpc::Status TestFiberSeqLock(::trpc::ServerContextPtr context,
                                  const ::trpc::testing::fibersvr::TestRequest* request,
                                  ::trpc::testing::fibersvr::TestReply* response) {
    if (request->msg() == "normal") {
      ::trpc::FiberLatch l(2);
      int shared_data = -1;
      int result = -1;
      ::trpc::FiberSeqLock seq_lock;

      // Reader
      trpc::StartFiberDetached([&] {
        unsigned seq;
        do {
          seq = seq_lock.ReadSeqBegin();
          // Reader logic
          result = shared_data;
        } while (seq_lock.ReadSeqRetry(seq));
        l.CountDown();
      });

      // Writer priority
      trpc::StartFiberDetached([&] {
        seq_lock.WriteSeqLock();
        // Writer logic
        shared_data = 1;
        seq_lock.WriteSeqUnlock();
        l.CountDown();
      });

      l.Wait();
      response->set_msg(std::to_string(result));
      return ::trpc::kSuccStatus;
    }

    return ::trpc::kUnknownErrorStatus;
  }

  ::trpc::Status TestFiberSharedMutex(::trpc::ServerContextPtr context,
                                      const ::trpc::testing::fibersvr::TestRequest* request,
                                      ::trpc::testing::fibersvr::TestReply* response) {
    if (request->msg() == "write_read") {
      // Simultaneous presence of readers and writers, with priority given to writers
      std::string shared_data = "";
      std::string result = "";
      ::trpc::FiberSharedMutex rwlock;
      ::trpc::Fiber write_fiber = ::trpc::Fiber([&rwlock, &shared_data] {
        // Call lock() during construction and unlock() during destruction
        std::scoped_lock _(rwlock);

        shared_data = "write";
      });

      ::trpc::Fiber read_fiber = ::trpc::Fiber([&rwlock, &shared_data, &result] {
        // Call lock_shared() during construction and unlock_shared() during destruction
        std::shared_lock _(rwlock);

        result = shared_data;
      });

      write_fiber.Join();
      read_fiber.Join();

      response->set_msg(result);
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "read_read") {
      bool is_try_lock_shared_repeatly = false;
      ::trpc::FiberSharedMutex rwlock;
      ::trpc::Fiber read_fiber1 = ::trpc::Fiber([&rwlock, &is_try_lock_shared_repeatly] {
        // Call lock_shared() during construction and unlock_shared() during destruction
        std::shared_lock _(rwlock);

        is_try_lock_shared_repeatly = rwlock.try_lock_shared();
      });

      ::trpc::Fiber read_fiber2 = ::trpc::Fiber([&rwlock] {
        // Call lock_shared() during construction and unlock_shared() during destruction
        std::shared_lock _(rwlock);
      });

      read_fiber1.Join();
      read_fiber2.Join();

      response->set_msg(std::to_string(is_try_lock_shared_repeatly));
      return ::trpc::kSuccStatus;
    }

    return ::trpc::kUnknownErrorStatus;
  }

  ::trpc::Status TestFiberTimer(::trpc::ServerContextPtr context, const ::trpc::testing::fibersvr::TestRequest* request,
                                ::trpc::testing::fibersvr::TestReply* response) {
    if (request->msg() == "set_fiber_timer") {
      auto start = ::trpc::ReadSteadyClock();
      int sleep_time = -1;
      auto timer_id = ::trpc::SetFiberTimer(start + std::chrono::milliseconds(10), [&] {
        sleep_time = (::trpc::ReadSteadyClock() - start) / std::chrono::milliseconds(1);
      });

      ::trpc::FiberSleepFor(std::chrono::milliseconds(100));

      // Release the timer resource
      KillFiberTimer(timer_id);

      response->set_msg(std::to_string(sleep_time));
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "set_fiber_periodic_timer") {
      auto start = ::trpc::ReadSteadyClock();
      std::atomic<std::size_t> called{};
      // Timed task that executes periodically
      auto timer_id = ::trpc::SetFiberTimer(start + std::chrono::milliseconds(10), std::chrono::milliseconds(10),
                                            [&] { ++called; });

      ::trpc::FiberSleepFor(std::chrono::milliseconds(100));

      // Release the timer resource
      KillFiberTimer(timer_id);

      response->set_msg(std::to_string(called));
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "fiber_timer_killer") {
      auto start = ::trpc::ReadSteadyClock();
      int sleep_time = -1;

      // FiberTimerKiller is used to wrap FiberTimer and automatically release the FiberTimer resource
      ::trpc::FiberTimerKiller killer(::trpc::SetFiberTimer(start + std::chrono::milliseconds(10), [&] {
        sleep_time = (::trpc::ReadSteadyClock() - start) / std::chrono::milliseconds(1);
      }));

      ::trpc::FiberSleepFor(std::chrono::milliseconds(100));

      response->set_msg(std::to_string(sleep_time));
      return ::trpc::kSuccStatus;
    }

    return ::trpc::kUnknownErrorStatus;
  }

  ::trpc::Status TestFiberBlockingBoundedQueue(::trpc::ServerContextPtr context,
                                               const ::trpc::testing::fibersvr::TestRequest* request,
                                               ::trpc::testing::fibersvr::TestReply* response) {
    if (request->msg() == "normal") {
      ::trpc::FiberBlockingBoundedQueue<int> queue{1024};
      int out = -1;
      ::trpc::Fiber fiber1 = ::trpc::Fiber([&queue] {
        ::trpc::FiberSleepFor(std::chrono::milliseconds(10));
        queue.Push(1);
      });

      ::trpc::Fiber fiber2 = ::trpc::Fiber([&queue, &out] { queue.Pop(out); });

      fiber1.Join();
      fiber2.Join();

      response->set_msg(std::to_string(out));
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "expires_in") {
      ::trpc::FiberBlockingBoundedQueue<int> queue{1024};
      int out = -1;
      ::trpc::Fiber fiber1 = ::trpc::Fiber([&queue] {
        ::trpc::FiberSleepFor(std::chrono::milliseconds(10));
        queue.Push(1, std::chrono::milliseconds(10));
      });

      ::trpc::Fiber fiber2 = ::trpc::Fiber([&queue, &out] { queue.Pop(out, std::chrono::milliseconds(100)); });

      fiber1.Join();
      fiber2.Join();

      response->set_msg(std::to_string(out));
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "expires_at") {
      ::trpc::FiberBlockingBoundedQueue<int> queue{1024};
      int out = -1;
      ::trpc::Fiber fiber1 = ::trpc::Fiber([&queue] {
        ::trpc::FiberSleepFor(std::chrono::milliseconds(10));
        queue.Push(1, ::trpc::ReadSteadyClock() + std::chrono::milliseconds(10));
      });

      ::trpc::Fiber fiber2 =
          Fiber([&queue, &out] { queue.Pop(out, ::trpc::ReadSteadyClock() + std::chrono::milliseconds(100)); });

      fiber1.Join();
      fiber2.Join();

      response->set_msg(std::to_string(out));
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "try") {
      ::trpc::FiberBlockingBoundedQueue<int> queue{1024};
      int out = -1;
      ::trpc::Fiber fiber1 = ::trpc::Fiber([&queue] { queue.TryPush(1); });

      ::trpc::Fiber fiber2 = ::trpc::Fiber([&queue, &out] {
        ::trpc::FiberSleepFor(std::chrono::milliseconds(10));
        queue.TryPop(out);
      });

      fiber1.Join();
      fiber2.Join();

      response->set_msg(std::to_string(out));
      return ::trpc::kSuccStatus;
    }

    return ::trpc::kUnknownErrorStatus;
  }

  ::trpc::Status TestFiberBlockingNoncontiguousBuffer(::trpc::ServerContextPtr context,
                                                      const ::trpc::testing::fibersvr::TestRequest* request,
                                                      ::trpc::testing::fibersvr::TestReply* response) {
    if (request->msg() == "normal") {
      FiberBlockingNoncontiguousBuffer buffer{1024};
      std::string out = "";
      ::trpc::Fiber fiber1 = ::trpc::Fiber([&buffer] {
        ::trpc::FiberSleepFor(std::chrono::milliseconds(10));
        ::trpc::NoncontiguousBuffer item;
        item.Append(::trpc::CreateBufferSlow("1"));
        buffer.Append(std::move(item));
      });

      ::trpc::Fiber fiber2 = ::trpc::Fiber([&buffer, &out] {
        ::trpc::NoncontiguousBuffer cut_item;
        buffer.Cut(cut_item, 1);
        out = ::trpc::FlattenSlow(cut_item);
      });

      fiber1.Join();
      fiber2.Join();

      response->set_msg(out);
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "expires_in") {
      FiberBlockingNoncontiguousBuffer buffer{1024};
      std::string out = "";
      ::trpc::Fiber fiber1 = ::trpc::Fiber([&buffer] {
        ::trpc::FiberSleepFor(std::chrono::milliseconds(10));
        ::trpc::NoncontiguousBuffer item;
        item.Append(::trpc::CreateBufferSlow("1"));
        buffer.Append(std::move(item), std::chrono::milliseconds(10));
      });

      ::trpc::Fiber fiber2 = ::trpc::Fiber([&buffer, &out] {
        ::trpc::NoncontiguousBuffer cut_item;
        buffer.Cut(cut_item, 1, std::chrono::milliseconds(100));
        out = ::trpc::FlattenSlow(cut_item);
      });

      fiber1.Join();
      fiber2.Join();

      response->set_msg(out);
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "expires_at") {
      FiberBlockingNoncontiguousBuffer buffer{1024};
      std::string out = "";
      ::trpc::Fiber fiber1 = ::trpc::Fiber([&buffer] {
        ::trpc::FiberSleepFor(std::chrono::milliseconds(10));
        ::trpc::NoncontiguousBuffer item;
        item.Append(::trpc::CreateBufferSlow("1"));
        buffer.Append(std::move(item), ::trpc::ReadSteadyClock() + std::chrono::milliseconds(10));
      });

      ::trpc::Fiber fiber2 = ::trpc::Fiber([&buffer, &out] {
        ::trpc::NoncontiguousBuffer cut_item;
        buffer.Cut(cut_item, 1, ::trpc::ReadSteadyClock() + std::chrono::milliseconds(100));
        out = ::trpc::FlattenSlow(cut_item);
      });

      fiber1.Join();
      fiber2.Join();

      response->set_msg(out);
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "try") {
      FiberBlockingNoncontiguousBuffer buffer{1024};
      std::string out = "";
      ::trpc::Fiber fiber1 = ::trpc::Fiber([&buffer] {
        ::trpc::NoncontiguousBuffer item;
        item.Append(::trpc::CreateBufferSlow("1"));
        buffer.TryAppend(std::move(item));
      });

      ::trpc::Fiber fiber2 = ::trpc::Fiber([&buffer, &out] {
        ::trpc::FiberSleepFor(std::chrono::milliseconds(10));
        ::trpc::NoncontiguousBuffer cut_item;
        buffer.TryCut(cut_item, 1);
        out = ::trpc::FlattenSlow(cut_item);
      });

      fiber1.Join();
      fiber2.Join();

      response->set_msg(out);
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "append_always") {
      FiberBlockingNoncontiguousBuffer buffer{1024};
      std::string out = "";
      ::trpc::Fiber fiber1 = ::trpc::Fiber([&buffer] {
        ::trpc::NoncontiguousBuffer item;
        item.Append(::trpc::CreateBufferSlow("helloworld"));
        buffer.AppendAlways(std::move(item));
      });

      ::trpc::Fiber fiber2 = ::trpc::Fiber([&buffer, &out] {
        ::trpc::FiberSleepFor(std::chrono::milliseconds(10));
        ::trpc::NoncontiguousBuffer cut_item;
        buffer.TryCut(cut_item, 10);
        out = ::trpc::FlattenSlow(cut_item);
      });

      fiber1.Join();
      fiber2.Join();

      response->set_msg(out);
      return ::trpc::kSuccStatus;
    }

    return ::trpc::kUnknownErrorStatus;
  }

  ::trpc::Status TestFiberEvent(::trpc::ServerContextPtr context, const ::trpc::testing::fibersvr::TestRequest* request,
                                ::trpc::testing::fibersvr::TestReply* response) {
    if (request->msg() == "normal") {
      std::string result = "";

      // Event notifier, used to invoke fibers running on the framework thread from an external thread,
      // such as asynchronously accessing MySQL using a new physical thread and then waking up the fiber within
      auto ev = std::make_unique<::trpc::FiberEvent>();

      std::thread t([&] {
        // Run in out thread...

        result = "fiber event set success in new thread";

        // Set an event that can be called in the thread where the framework runs fibers or in the thread of the
        // business itself Wake up FiberEvent after completion.
        ev->Set();
      });

      ev->Wait();

      t.join();

      response->set_msg(result);
      return ::trpc::kSuccStatus;
    }

    return ::trpc::kUnknownErrorStatus;
  }

  ::trpc::Status TestFiberLocal(::trpc::ServerContextPtr context, const ::trpc::testing::fibersvr::TestRequest* request,
                                ::trpc::testing::fibersvr::TestReply* response) {
    if (request->msg() == "normal") {
      static ::trpc::FiberLocal<int> fls;

      ::trpc::Fiber fiber1 = ::trpc::Fiber([] { *fls = 1; });

      ::trpc::Fiber fiber2 = ::trpc::Fiber([] { *fls = 2; });

      fiber1.Join();
      fiber2.Join();

      response->set_msg(std::to_string(*fls));
      return ::trpc::kSuccStatus;
    }

    return ::trpc::kUnknownErrorStatus;
  }

  ::trpc::Status TestFiberFuture(::trpc::ServerContextPtr context,
                                 const ::trpc::testing::fibersvr::TestRequest* request,
                                 ::trpc::testing::fibersvr::TestReply* response) {
    if (request->msg() == "normal") {
      std::string result = "";
      ::trpc::Promise<std::string> promise;
      auto fut = promise.GetFuture();

      // Need to use the future approach to notify in a new thread
      std::thread t([p = std::move(promise)]() mutable {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        p.SetValue(std::string("set_value_ok"));
      });
      t.detach();

      // Fiber-level blocking
      auto valid_future = ::trpc::fiber::BlockingGet(std::move(fut));
      result = valid_future.GetValue0();

      response->set_msg(result);
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "try_get") {
      std::string result = "";
      ::trpc::Promise<std::string> promise;
      auto fut = promise.GetFuture();

      // Need to use the future approach to notify in a new thread
      std::thread t([p = std::move(promise)]() mutable {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        p.SetValue(std::string("set_value_ok_in_time"));
      });
      t.detach();

      // Awakened within the timeout period without timing out
      auto valid_future = ::trpc::fiber::BlockingTryGet(std::move(fut), 50);
      result = (*valid_future).GetValue0();

      response->set_msg(result);
      return ::trpc::kSuccStatus;
    }

    if (request->msg() == "try_get_timeout") {
      std::string result = "";
      ::trpc::Promise<std::string> promise;
      auto fut = promise.GetFuture();

      // Need to use the future approach to notify in a new thread
      std::thread t([p = std::move(promise)]() mutable {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        p.SetValue(std::string("set_value_timeout"));
      });
      t.detach();

      // Not awakened within the timeout period, triggering a timeout, and returning std::nullopt
      auto invalid_future = ::trpc::fiber::BlockingTryGet(std::move(fut), 50);
      if (invalid_future == std::nullopt) {
        response->set_msg(result);
        return ::trpc::kSuccStatus;
      }
    }

    return ::trpc::kUnknownErrorStatus;
  }

  ::trpc::Status TestFiberAsync(::trpc::ServerContextPtr context, const ::trpc::testing::fibersvr::TestRequest* request,
                                ::trpc::testing::fibersvr::TestReply* response) {
    if (request->msg() == "normal") {
      // Return a Future using Async
      int result = 0;
      std::vector<::trpc::Future<int>> futs;
      futs.emplace_back(::trpc::fiber::Async([&] { return 1; }));
      futs.emplace_back(::trpc::fiber::Async([&] { return 2; }));
      futs.emplace_back(::trpc::fiber::Async([&] { return 3; }));

      std::thread t([&] {
        trpc::WhenAll(futs.begin(), futs.end()).Then([&result](std::vector<trpc::Future<int>>&& vec_futs) {
          // Check and retrieve the results of each operation
          int aysnc_value0 = vec_futs[0].GetValue0();
          int aysnc_value1 = vec_futs[1].GetValue0();
          int aysnc_value2 = vec_futs[2].GetValue0();

          result = aysnc_value0 + aysnc_value1 + aysnc_value2;

          return MakeReadyFuture<>();
        });
      });

      t.join();

      response->set_msg(std::to_string(result));
      return ::trpc::kSuccStatus;
    }

    return ::trpc::kUnknownErrorStatus;
  }
};

int FiberServer::Initialize() {
  RegisterService("fiber_service", std::make_shared<FiberServerImpl>());

  server_->Start();

  test_signal_->SignalClientToContinue();

  return 0;
}

extern "C" void __gcov_flush();

void FiberServer::Destroy() { __gcov_flush(); }

}  // namespace trpc::testing
