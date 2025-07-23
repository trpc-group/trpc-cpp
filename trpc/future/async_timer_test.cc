//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "trpc/future/async_timer.h"

#include <chrono>
#include <condition_variable>
#include <tuple>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/future/future_utility.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/runtime.h"
#include "trpc/runtime/separate_runtime.h"

namespace trpc::testing {

constexpr char kInstanceName[] = "default_instance";

class TestAsyncTimer : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    EXPECT_EQ(TrpcConfig::GetInstance()->Init("trpc/runtime/threadmodel/testing/merge.yaml"), 0);
    merge::StartRuntime();
  }

  static void TearDownTestCase() {
    merge::TerminateRuntime();
  }
};

TEST_F(TestAsyncTimer, Create) {
  auto counter = std::make_shared<int>(0);

  {  // invalid
    auto timer = iotimer::Create(100, 0, [counter]() { ++(*counter); });
    EXPECT_EQ(timer, kInvalidTimerId);
  }

  Reactor* reactor = runtime::GetReactor(0, kInstanceName);
  reactor->SubmitTask([counter]() {
    auto timer = iotimer::Create(100, 100, [counter]() { ++(*counter); });
    EXPECT_NE(timer, kInvalidTimerId);
    iotimer::Detach(timer);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  EXPECT_NEAR(*counter, 5, 1);
}

TEST_F(TestAsyncTimer, Cancel) {
  auto counter = std::make_shared<int>(0);

  {  // invalid
    EXPECT_FALSE(iotimer::Cancel(kInvalidTimerId));
  }

  Reactor* reactor = runtime::GetReactor(0, kInstanceName);
  reactor->SubmitTask([counter]() {
    auto timer = iotimer::Create(100, 100, [counter]() { ++(*counter); });
    EXPECT_NE(timer, kInvalidTimerId);

    EXPECT_TRUE(iotimer::Cancel(timer));
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  EXPECT_EQ(*counter, 0);
}

TEST_F(TestAsyncTimer, PauseResume) {
  {  // invalid
    EXPECT_FALSE(iotimer::Pause(kInvalidTimerId));
    EXPECT_FALSE(iotimer::Resume(kInvalidTimerId));
  }

  auto counter = std::make_shared<int>(0);
  auto timer = std::make_shared<uint64_t>(kInvalidTimerId);

  Reactor* reactor = runtime::GetReactor(0, kInstanceName);
  reactor->SubmitTask([counter, timer]() {
    *timer = iotimer::Create(100, 100, [counter]() { ++(*counter); });
    EXPECT_NE(*timer, kInvalidTimerId);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_NEAR(*counter, 3, 1);

  reactor->SubmitTask([timer]() { EXPECT_TRUE(iotimer::Pause(*timer)); });

  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_NEAR(*counter, 3, 1);

  reactor->SubmitTask([timer]() { EXPECT_TRUE(iotimer::Resume(*timer)); });
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  EXPECT_NEAR(*counter, 6, 1);

  bool cancel = false;
  reactor->SubmitTask([timer, &cancel]() {
    iotimer::Cancel(*timer);
    cancel = true;
  });

  while (!cancel) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  EXPECT_TRUE(cancel);
}

TEST_F(TestAsyncTimer, After) {
  bool flag = false;
  auto timer = std::make_shared<AsyncTimer>();
  Reactor::Task timer_after_task = [&flag, &timer] {
    auto call_thread_id = std::this_thread::get_id();
    std::cout << "call_thread_id:" << call_thread_id << ",call_back_thread_id:" << std::this_thread::get_id()
                << std::endl;
    auto ft = timer->After(100).Then([call_thread_id, &flag, &timer]() {
      std::cout << "timer call_thread_id:" << call_thread_id << ",call_back_thread_id:" << std::this_thread::get_id()
                << std::endl;
      EXPECT_EQ(call_thread_id, std::this_thread::get_id());
      flag = true;
      auto fut = MakeReadyFuture<bool>(true);
      return fut;
    });
  };

  Reactor* reactor = runtime::GetReactor(0, std::string(kInstanceName));
  reactor->SubmitTask(std::move(timer_after_task));

  while (!flag) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  EXPECT_TRUE(flag);
}

TEST_F(TestAsyncTimer, AfterAndDetach) {
  bool flag = false;
  Reactor::Task timer_after_task = [&flag] {
    auto timer = std::make_shared<AsyncTimer>();
    auto call_thread_id = std::this_thread::get_id();
    std::cout << "call_thread_id:" << call_thread_id << ",call_back_thread_id:" << std::this_thread::get_id()
                << std::endl;
    timer->After(100).Then([call_thread_id, &flag, timer]() {
      std::cout << "timer call_thread_id:" << call_thread_id << ",call_back_thread_id:" << std::this_thread::get_id()
                << std::endl;
      EXPECT_EQ(call_thread_id, std::this_thread::get_id());
      flag = true;
      return MakeReadyFuture<bool>(true);
    });

    timer->Detach();
  };

  Reactor* reactor = runtime::GetReactor(0, std::string(kInstanceName));
  reactor->SubmitTask(std::move(timer_after_task));

  while (!flag) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  EXPECT_TRUE(flag);
}

TEST_F(TestAsyncTimer, CancelBeforeReachTime) {
  std::shared_ptr<AsyncTimer> timer = std::make_shared<AsyncTimer>(false);
  Future<> ft;
  Reactor::Task timer_after_task = [&ft, &timer] { ft = timer->After(10000); };
  Reactor* reactor = runtime::GetReactor(0, kInstanceName);
  reactor->SubmitTask(std::move(timer_after_task));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  Reactor::Task cancel_timer_task = [timer] { timer->Cancel(); };
  reactor->SubmitTask(std::move(cancel_timer_task));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_TRUE(!ft.IsReady());
  EXPECT_TRUE(ft.IsFailed());
  EXPECT_EQ(std::string(ft.GetException().what()), std::string(AsyncTimer::Cancelled().what()));
  std::cout << "GetException:" << ft.GetException().what() << std::endl;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(TestAsyncTimer, CancelAfterReachTime) {
  std::shared_ptr timer = std::make_shared<AsyncTimer>(false);

  Reactor::Task timer_after_task = [&timer] {
    auto call_thread_id = std::this_thread::get_id();

    auto ft = timer->After(100).Then([call_thread_id, &timer]() {
      std::cout << "call_thread_id:" << call_thread_id << ",call_back_thread_id:" << std::this_thread::get_id()
                << std::endl;
      EXPECT_EQ(call_thread_id, std::this_thread::get_id());

      return MakeReadyFuture<bool>(true);
    });
  };

  Reactor* reactor = runtime::GetReactor(0, kInstanceName);
  reactor->SubmitTask(std::move(timer_after_task));

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  Reactor::Task cancel_timer_task = [timer] { timer->Cancel(); };
  reactor->SubmitTask(std::move(cancel_timer_task));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(TestAsyncTimer, AfterWithOnceExecutorSubmitTask) {
  std::shared_ptr once_timer = std::make_shared<AsyncTimer>();
  Reactor::Task add_once_timer_task = [&once_timer]() {
    auto call_thread_id = std::this_thread::get_id();
    auto executor = [call_thread_id, &once_timer]() {
      std::cout << "call_thread_id:" << call_thread_id << ",call_back_thread_id:" << std::this_thread::get_id()
                << std::endl;
      EXPECT_EQ(call_thread_id, std::this_thread::get_id());
    };

    auto ret = once_timer->After(100, std::move(executor));
    EXPECT_TRUE(ret);

    ret = once_timer->After(100, []() {});
    EXPECT_FALSE(ret);
  };

  Reactor* reactor = runtime::GetReactor(0, kInstanceName);
  reactor->SubmitTask(std::move(add_once_timer_task));

  std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

TEST_F(TestAsyncTimer, CancelOnceTimerBeforeReachTime) {
  std::shared_ptr once_timer = std::make_shared<AsyncTimer>(false);
  bool once_timer_executed{false};
  Reactor::Task add_once_timer_task = [&once_timer_executed, &once_timer] {
    once_timer->After(10000, [&once_timer_executed]() { once_timer_executed = true; });
  };
  Reactor* reactor = runtime::GetReactor(0, kInstanceName);
  reactor->SubmitTask(std::move(add_once_timer_task));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  Reactor::Task cancel_once_timer_task = [once_timer] { once_timer->Cancel(); };
  reactor->SubmitTask(std::move(cancel_once_timer_task));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  EXPECT_FALSE(once_timer_executed);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(TestAsyncTimer, CancelOnceTimerAfterReachTime) {
  std::condition_variable cond_timer_job_executed;
  std::mutex timer_job_mutex;

  std::shared_ptr once_timer = std::make_shared<AsyncTimer>(false);
  bool once_timer_executed{false};
  Reactor::Task add_once_timer_task = [&once_timer_executed, &once_timer, &cond_timer_job_executed, &timer_job_mutex] {
    auto call_thread_id = std::this_thread::get_id();
    auto executor = [call_thread_id, &once_timer_executed, &cond_timer_job_executed, &timer_job_mutex]() {
      std::unique_lock lock(timer_job_mutex);
      once_timer_executed = true;
      cond_timer_job_executed.notify_all();

      std::cout << "call_thread_id:" << call_thread_id << ",call_back_thread_id:" << std::this_thread::get_id()
                << std::endl;
      EXPECT_EQ(call_thread_id, std::this_thread::get_id());
    };

    once_timer->After(100, std::move(executor));
  };

  Reactor* reactor = runtime::GetReactor(0, kInstanceName);
  reactor->SubmitTask(std::move(add_once_timer_task));

  while (!once_timer_executed) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  EXPECT_TRUE(once_timer_executed);

  bool cancel = false;
  Reactor::Task cancel_once_timer_task = [&cancel, once_timer] {
    once_timer->Cancel();
    cancel = true;
  };
  reactor->SubmitTask(std::move(cancel_once_timer_task));

  while (!cancel) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

TEST_F(TestAsyncTimer, AfterWithCycleExecutorSubmitTask) {
  auto promise = std::make_shared<Promise<>>();
  auto timer_executed_times = std::make_shared<int>(0);

  std::shared_ptr cycle_timer = std::make_shared<AsyncTimer>();
  Reactor::Task add_once_timer_task = [cycle_timer, timer_executed_times, promise]() {
    auto call_thread_id = std::this_thread::get_id();
    auto executor = [call_thread_id, cycle_timer, timer_executed_times, promise]() {
      std::cout << "cycle timer, call_thread_id:" << call_thread_id
                << ",call_back_thread_id:" << std::this_thread::get_id() << std::endl;
      EXPECT_EQ(call_thread_id, std::this_thread::get_id());
      ++(*timer_executed_times);

      if (*timer_executed_times > 2) {
        cycle_timer->Cancel();
        promise->SetValue();
      }
    };

    auto ret = cycle_timer->After(100, std::move(executor), 100);
    EXPECT_TRUE(ret);

    ret = cycle_timer->After(100, []() {});
    EXPECT_FALSE(ret);
  };

  Reactor* reactor = runtime::GetReactor(0, kInstanceName);
  reactor->SubmitTask(std::move(add_once_timer_task));
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  auto future = promise->GetFuture();
  future::BlockingGet(std::move(future));

  EXPECT_GT(*timer_executed_times, 1);

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  EXPECT_LT(*timer_executed_times, 10);
}

}  // namespace trpc::testing
