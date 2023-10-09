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

#include "trpc/future/future.h"

#include <chrono>
#include <string>
#include <thread>

#include "gtest/gtest.h"

#include "trpc/future/executor.h"
#include "trpc/util/thread/latch.h"

namespace trpc {

// Subclass of ExceptionBase.
class TestException : public ExceptionBase {
 public:
  const char* what() const noexcept override { return "test exception"; }
};

// Test ready future Then by callback with value parameter and return void.
TEST(Future, ThenWithValueReadyNoReturn) {
  auto flag = std::make_shared<bool>(false);
  auto depth = std::make_shared<int>(0);
  MakeReadyFuture<int>(20190820).Then([flag](int&& x) {
    *flag = true;
    EXPECT_EQ(x, 20190820);
  });

  EXPECT_EQ(*flag, true);
}

// Test ready future Then by callback with value parameter and return next future.
TEST(Future, ThenWithValueReady) {
  auto flag = std::make_shared<bool>(false);
  MakeReadyFuture<int>(20190820).Then([flag](int&& x) {
    *flag = true;
    EXPECT_EQ(x, 20190820);
    return MakeReadyFuture<>();
  });

  EXPECT_EQ(*flag, true);
}

// Test exceptional future Then by callback with value parameter and return next future.
TEST(Future, ThenWithValueFailed) {
  auto flag = std::make_shared<bool>(false);
  MakeExceptionFuture<int>(TestException()).Then([flag](int&& x) {
    *flag = true;
    return MakeReadyFuture<>();
  });

  EXPECT_EQ(*flag, false);
}

// Test exceptional future Then by callback with value parameter and return void.
TEST(Future, ThenWithValueFailedNoReturn) {
  auto flag = std::make_shared<bool>(false);
  MakeExceptionFuture<int>(TestException()).Then([flag](int&& x) {
    *flag = true;
  });

  EXPECT_EQ(*flag, false);
}

// Test GetValue inside callback return next future.
TEST(Future, ThenWithFutureReadyUsingGetValue) {
  auto flag = std::make_shared<bool>(false);
  MakeReadyFuture<int>(20190820).Then([flag](Future<int>&& x) {
    *flag = true;
    EXPECT_EQ(x.IsReady(), true);
    EXPECT_EQ(x.IsFailed(), false);
    auto value = std::get<0>(x.GetValue());
    EXPECT_EQ(value, 20190820);
    return MakeReadyFuture<>();
  });

  EXPECT_EQ(*flag, true);
}

// Test GetValue inside callback return void.
TEST(Future, ThenWithFutureReadyUsingGetvalueNoReturn) {
  auto flag = std::make_shared<bool>(false);
  MakeReadyFuture<int>(20190820).Then([flag](Future<int>&& x) {
    *flag = true;
    EXPECT_EQ(x.IsReady(), true);
    EXPECT_EQ(x.IsFailed(), false);
    auto value = std::get<0>(x.GetValue());
    EXPECT_EQ(value, 20190820);
  });

  EXPECT_EQ(*flag, true);
}

// Test GetValue0 inside callback return next future.
TEST(Future, ThenWithFutureReadyUsingGetvalueHp) {
  auto flag = std::make_shared<bool>(false);
  MakeReadyFuture<int>(20190820).Then([flag](Future<int>&& x) {
    *flag = true;
    EXPECT_EQ(x.IsReady(), true);
    EXPECT_EQ(x.IsFailed(), false);
    auto value = x.GetValue0();
    EXPECT_EQ(value, 20190820);
    return MakeReadyFuture<>();
  });

  EXPECT_EQ(*flag, true);
}

// Test GetValue0 inside callback return void.
TEST(Future, ThenWithFutureReadyUsingGetvalueHpNoReturn) {
  auto flag = std::make_shared<bool>(false);
  MakeReadyFuture<int>(20190820).Then([flag](Future<int>&& x) {
    *flag = true;
    EXPECT_EQ(x.IsReady(), true);
    EXPECT_EQ(x.IsFailed(), false);
    auto value = x.GetValue0();
    EXPECT_EQ(value, 20190820);
  });

  EXPECT_EQ(*flag, true);
}

// Test exception future Then by callback parametered with future and return future inside.
TEST(Future, ThenWithFutureFailed) {
  auto flag = std::make_shared<bool>(false);
  MakeExceptionFuture<int>(TestException()).Then([flag](Future<int>&& x) {
    *flag = true;
    EXPECT_EQ(x.IsReady(), false);
    EXPECT_EQ(x.IsFailed(), true);
    auto exception = x.GetException();
    EXPECT_EQ(exception.is(TestException()), true);
    return MakeReadyFuture<>();
  });

  EXPECT_EQ(*flag, true);
}

// Test exceptional future Then by callback parametered with future and return void.
TEST(Future, ThenWithFutureFailedNoReturn) {
  auto flag = std::make_shared<bool>(false);
  MakeExceptionFuture<int>(TestException()).Then([flag](Future<int>&& x) {
    *flag = true;
    EXPECT_EQ(x.IsReady(), false);
    EXPECT_EQ(x.IsFailed(), true);
    auto exception = x.GetException();
    EXPECT_EQ(exception.is(TestException()), true);
  });

  EXPECT_EQ(*flag, true);
}

// Test exceptional future Then by callback parametered with value and return future.
TEST(Future, TestException) {
  auto fut = MakeExceptionFuture<>(TestException()).Then([]() {
    ADD_FAILURE();
    return MakeReadyFuture<bool>(true);
  });

  EXPECT_TRUE(fut.IsFailed());
  EXPECT_FALSE(fut.IsReady());
  EXPECT_TRUE(fut.GetException().is<TestException>());
}

// Test exceptional future Then by callback parametered with value and passed by.
TEST(Future, TestExceptionPropagate) {
  auto flag = std::make_shared<bool>(false);
  MakeExceptionFuture<int>(TestException())
    .Then([](int&& x) {
      ADD_FAILURE();
      return MakeReadyFuture<int>(123);
    })
    .Then([flag](Future<int>&& x) {
      *flag = true;
      EXPECT_EQ(x.IsReady(), false);
      EXPECT_EQ(x.IsFailed(), true);
      auto exception = x.GetException();
      EXPECT_EQ(exception.is(TestException()), true);
      return MakeReadyFuture<>();
    });

  EXPECT_EQ(*flag, true);
}

// Chain multiple Then.
Future<int> ALongChain(Future<>&& fut) {
  return MakeReadyFuture<>()
    .Then([fut = std::move(fut)]() mutable {
      return fut.Then([]() { return MakeReadyFuture<>().Then([]() { return MakeReadyFuture<int>(1234567321); }); });
    })
    .Then([](int&& v) {
      return MakeReadyFuture<int>(std::move(v));
    });
}

// Test long chain Then by callback with return.
TEST(Future, MoveState) {
  Promise<> p;
  auto flag = std::make_shared<bool>(false);
  auto fut = ALongChain(p.GetFuture());
  p.SetValue();
  fut.Then([flag](int&& v) {
    EXPECT_EQ(v, 1234567321);
    *flag = true;
    return MakeReadyFuture<>();
  });

  EXPECT_EQ(*flag, true);
}

// Test long chain Then by callback without return.
TEST(Future, MoveStateNoReturn) {
  Promise<> p;
  auto flag = std::make_shared<bool>(false);
  auto fut = ALongChain(p.GetFuture());
  p.SetValue();
  fut.Then([flag](int&& v) {
    EXPECT_EQ(v, 1234567321);
    *flag = true;
  });

  EXPECT_EQ(*flag, true);
}

// Test Then inside different level with return.
TEST(Future, AnotherMoveState) {
  Promise<> p;
  auto flag = std::make_shared<bool>(false);
  auto fut = ALongChain(p.GetFuture());
  p.SetValue();
  auto fut_ptr = std::make_shared<Future<int>>(std::move(fut));
  MakeReadyFuture<>().Then([flag, fut_ptr]() {
    return fut_ptr->Then([flag](int&& v) {
      EXPECT_EQ(v, 1234567321);
      *flag = true;
      return MakeReadyFuture<>();
    });
  });

  EXPECT_EQ(*flag, true);
}

// Test Then inside different level without return.
TEST(Future, AnotherMoveStateNoReturn) {
  Promise<> p;
  auto flag = std::make_shared<bool>(false);
  auto fut = ALongChain(p.GetFuture());
  p.SetValue();
  auto fut_ptr = std::make_shared<Future<int>>(std::move(fut));
  MakeReadyFuture<>()
    .Then([flag, fut_ptr]() {
      fut_ptr->Then([flag](int&& v) {
        EXPECT_EQ(v, 1234567321);
        *flag = true;
        return MakeReadyFuture<int>(11);
      })
      .Then([flag](int&& v) {
        EXPECT_EQ(v, 11);
        *flag = true;
      });
    });

  EXPECT_EQ(*flag, true);
}

Future<int> func1() {
  return MakeReadyFuture<int>(2);
}

Future<int> func2() {
  Promise<int> p1;
  auto fut = p1.GetFuture();
  std::thread t([p = std::move(p1)]() mutable {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    p.SetValue(std::tuple<int>(3));
  });
  t.detach();
  return fut;
}

Future<int> func3() {
  Promise<int> p1;
  auto fut = p1.GetFuture();
  std::thread t([p = std::move(p1)]() mutable {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    p.SetException(CommonException("return exception"));
  });
  t.detach();
  return fut;
}

Future<int> func4() {
  Promise<int> p1;
  auto fut = p1.GetFuture();
  std::thread t([p = std::move(p1)]() mutable {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  });
  t.detach();
  return fut;
}

// Test random chain.
TEST(Future, RandomTest) {
  bool flag = false;
  Latch latch(1);
  MakeReadyFuture<int>(1)
    .Then([](int&& x) {
      std::cout << "enter continuation 1" << std::endl;
      return func1().Then([](int&& x) {
        std::cout << "enter continuation 2" << std::endl;
        return func2().Then([=](int&& x) {
          std::cout << "enter continuation 3" << std::endl;
          return MakeReadyFuture<int>(4);
        });
      });
    })
    .Then([&](int&& y) {
      std::cout << "enter continuation 4" << std::endl;
      flag = true;
      latch.count_down();
      return MakeReadyFuture<>();
    });
  latch.wait();

  EXPECT_EQ(flag, true);
}

// Test random chain.
TEST(Future, RandomTest1) {
  bool flag = false;
  Latch latch(1);
  func2()
    .Then([](int&& x) {
      return func2()
        .Then([=](int&& x) {
          std::cout << "enter continuation 3" << std::endl;
          return MakeReadyFuture<int>(4);
        });
    })
    .Then([&](int&& y) {
      std::cout << "enter continuation 4" << std::endl;
      flag = true;
      latch.count_down();
      return MakeReadyFuture<>();
    });

  latch.wait();
  EXPECT_EQ(flag, true);
}

// Test random chain.
TEST(Future, RandomTest2) {
  bool flag = false;
  Latch latch(1);
  func2()
    .Then([](int&& x) {
      return func2()
        .Then([=](int&& x) {
          std::cout << "enter continuation 3" << std::endl;
          return MakeReadyFuture<int>(4);
        });
    })
    .Then([&](int&& y) {
      std::cout << "enter continuation 4" << std::endl;
      flag = true;
      latch.count_down();
      return MakeReadyFuture<>();
    });

  latch.wait();
  EXPECT_EQ(flag, true);
}

// Test random chain.
TEST(Future, RandomTest3) {
  bool flag = false;
  auto fut = MakeReadyFuture<int>(1)
    .Then([=](int&& x) {
      std::cout << "enter continuation 1" << std::endl;
      return func1().Then([](int&& x) {
        std::cout << "enter continuation 2" << std::endl;
        return func2().Then([=](int&& x) {
          std::cout << "enter continuation 3" << std::endl;
          return func2().Then([](int&& x) {
            return func2().Then([](int&& x) {
              return MakeReadyFuture<int>(5);
            });
          });
        });
      });
    });

  Latch latch(1);
  fut.Then([&](int&& y) {
    EXPECT_EQ(y, 5);
    std::cout << "enter continuation 4" << std::endl;
    flag = true;
    latch.count_down();
    return MakeReadyFuture<>();
  });
  latch.wait();
  EXPECT_EQ(flag, true);
}

// Test future wait.
TEST(Future, SyncWait) {
  auto fut = func2();
  bool ret = fut.Wait();
  EXPECT_EQ(3, std::get<0>(fut.GetValue()));
  EXPECT_TRUE(ret);

  fut = func3();
  ret = fut.Wait();
  EXPECT_TRUE(ret);
  std::cout << fut.GetException().what() << std::endl;

  fut = func4();
  ret = fut.Wait(200);
  EXPECT_TRUE(ret);

  fut = func4();
  ret = fut.Wait(50);
  EXPECT_FALSE(ret);

  fut = func2().Then([](int&& val) { return func1(); });
  ret = fut.Wait();
  EXPECT_EQ(2, std::get<0>(fut.GetValue()));

  fut = func1().Then([](int&& val) { return func3(); });
  ret = fut.Wait();
  std::cout << fut.GetException().what() << std::endl;
}

// Test throw.
TEST(Future, ContinuationReturn) {
  auto func = []() {
    Latch latch(1);
    auto fut = MakeReadyFuture<>().Then([&]() {
      throw std::runtime_error("aa");
      latch.count_down();
      return MakeReadyFuture<>();
    });
    ADD_FAILURE();
    latch.wait();
    EXPECT_TRUE(fut.IsReady());
  };

  EXPECT_THROW(func(), std::runtime_error);
}

// Test Set multiple times.
TEST(Promise, SheduleOnce) {
  auto pr = Promise<>();
  auto fut = pr.GetFuture().Then([](Future<>&& fut) {
    if (fut.IsReady())
      return MakeReadyFuture<>();
    else
      return MakeExceptionFuture<>(fut.GetException());
  });

  pr.SetException(CommonException("test"));
  pr.SetValue();

  EXPECT_EQ(fut.IsReady(), false);
  EXPECT_EQ(fut.IsFailed(), true);
}

// Test set before Then.
TEST(Promise, ReadyFailedSchedule) {
  auto pr = Promise<>();
  pr.SetException(CommonException("test"));
  pr.SetValue();

  auto fut = pr.GetFuture().Then([](Future<>&& fut) {
    if (fut.IsReady())
      return MakeReadyFuture<>();
    else
      return MakeExceptionFuture<>(fut.GetException());
  });

  EXPECT_EQ(fut.IsReady(), true);
  EXPECT_EQ(fut.IsFailed(), false);
}

// Test chained callback parametered with multiple values.
TEST(Promise, MultiParamWithContinuationValue) {
  Promise<int, std::string> pr;
  bool success = false;
  auto fut = pr.GetFuture();
  fut.Then([&success](int&& integer, std::string&& str) {
    EXPECT_EQ(integer, 1);
    EXPECT_EQ(str, "hello");
    success = true;
    return trpc::MakeReadyFuture<>();
  });

  pr.SetValue(1, "hello");
  EXPECT_EQ(success, true);
}

// Test chained callback parametered with multiple parameters future.
TEST(Promise, MultiParamWithContinuationFuture) {
  Promise<int, std::string> pr;
  bool success = false;
  auto fut = pr.GetFuture();
  fut.Then([&success](Future<int, std::string>&& fut) {
    EXPECT_TRUE(fut.IsReady());
    auto value = fut.GetValue();
    EXPECT_EQ(std::get<0>(value), 1);
    EXPECT_EQ(std::get<1>(value), "hello");
    success = true;
    return trpc::MakeReadyFuture<>();
  });

  pr.SetValue(1, "hello");
  EXPECT_EQ(success, true);
}

// Test user executor.
TEST(Future, ThenExecutor) {
  /// @brief Create a new thread for every future task.
  class ThreadExecutor : public Executor {
   public:
    bool SubmitTask(Task&& task) override {
      std::thread(std::move(task)).detach();
      return true;
    }
  };

  auto executor = std::make_unique<ThreadExecutor>();
  int counter = 0;
  auto fut = MakeReadyFuture<std::thread::id>(std::this_thread::get_id())
    .Then(executor.get(), [&counter](std::thread::id&& caller) {
      ++counter;
      EXPECT_NE(std::this_thread::get_id(), caller);
      return MakeReadyFuture<std::thread::id>(std::this_thread::get_id());
    })
    .Then(executor.get(), [&counter](Future<std::thread::id>&& fut) {
      ++counter;
      EXPECT_TRUE(fut.IsReady());
      EXPECT_NE(std::this_thread::get_id(), std::get<0>(fut.GetValue()));
      return MakeReadyFuture<std::thread::id>(std::this_thread::get_id());
    })
    .Then([&counter](Future<std::thread::id>&& fut) {
      ++counter;
      EXPECT_TRUE(fut.IsReady());
      // Use default InlineExecutor.
      EXPECT_EQ(std::this_thread::get_id(), std::get<0>(fut.GetValue()));
      return MakeExceptionFuture<>(CommonException("just joke"));
    })
    .Then([&counter](Future<>&& fut) {
      ++counter;
      EXPECT_TRUE(fut.IsFailed());
      return MakeReadyFuture<bool>(true);
    });

  while (!(fut.IsReady() || fut.IsFailed())) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  EXPECT_EQ(fut.IsReady(), true);
  EXPECT_EQ(std::get<0>(fut.GetValue()), true);
  EXPECT_EQ(counter, 4);
}

// Test shared ptr user executor.
TEST(Future, ThenSharedPtrExecutor) {
  // Randomly execute future task in current thread or using default executor.
  class DummyExecutor : public Executor {
   public:
    explicit DummyExecutor(bool& alive) : alive_(alive) { alive = true; }
    ~DummyExecutor() { alive_ = false; }
    bool SubmitTask(Task&& task) override {
      if (ret_) {
        task();
      }
      return ret_;
    }
    void SetRet(bool ret) { ret_ = ret; }

   private:
    bool& alive_;
    bool ret_{true};
  };

  bool alive = false;
  auto executor = std::make_shared<DummyExecutor>(alive);
  EXPECT_TRUE(alive);

  executor->SetRet(false);
  MakeReadyFuture<>().Then(executor.get(), [&alive]() {
    alive = true;
    return MakeReadyFuture<>();
  });
  EXPECT_TRUE(alive);

  executor->SetRet(true);
  MakeReadyFuture<>().Then(std::move(executor.get()), [&alive]() {
    alive = false;
    return MakeReadyFuture<>();
  });
  EXPECT_FALSE(alive);
}

// Test copyable executor.
TEST(Future, ThenCopyableExecutor) {
  // For test executor copyable.
  class DummyExecutor : public Executor {
   public:
    explicit DummyExecutor(int& alive) : alive_(alive) { ++alive; }
    ~DummyExecutor() { --alive_; }
    bool SubmitTask(Task&& task) override { return true; }
    DummyExecutor* operator->() const { return const_cast<DummyExecutor*>(this); }

    DummyExecutor(const DummyExecutor& other) : alive_(other.alive_) { ADD_FAILURE(); }
    DummyExecutor(DummyExecutor&& other) : alive_(other.alive_) { ++alive_; }

   private:
    int& alive_;
  };

  int alive = 0;
  DummyExecutor executor(alive);
  EXPECT_EQ(alive, 1);

  MakeReadyFuture<>().Then(&executor, []() { return MakeReadyFuture<>(); });
  EXPECT_EQ(alive, 1);
}

// Test mltiple threads executor.
TEST(Future, TestExecuteOkInDifferentThread) {
  static int exec_count = 0;
  int loop_times = 50000;
  for (int i = 0; i < loop_times; i++) {
    Promise<int> pr;
    auto fut = pr.GetFuture();
    std::thread t([pr = std::move(pr)]() mutable {
      std::this_thread::sleep_for(std::chrono::nanoseconds(1));
      pr.SetValue(1);
    });
    std::this_thread::sleep_for(std::chrono::nanoseconds(10000));
    fut.Then([](int&& val) {
      exec_count++;
      return MakeReadyFuture<>();
    });
    t.join();
  }

  ASSERT_EQ(exec_count, loop_times) << exec_count << std::endl;
}

// Test promise by Then callback parameterd with multiple values and return void.
TEST(Promise, MultiParamWithNoReturn) {
  Promise<int, std::string> pr;
  bool success = false;
  auto fut = pr.GetFuture();
  fut.Then([&success](int&& integer, std::string&& str) {
    EXPECT_EQ(integer, 1);
    EXPECT_EQ(str, "hello");
    success = true;
    return trpc::MakeReadyFuture<std::string, int>("hello", 10086);
  }).Then([](std::string&& str, int&& x) {
    EXPECT_EQ(str, "hello");
    EXPECT_EQ(x, 10086);
  });

  pr.SetValue(1, "hello");
  EXPECT_EQ(success, true);
}

// Test future callback executed in designed thread.
TEST(Future, ContinuationOnCurrent1) {
  Promise<> pr;
  auto ft = pr.GetFuture();
  std::thread::id id;
  ft.Then([&id]() {
    EXPECT_EQ(std::this_thread::get_id(), id);
  });

  std::thread([&id, &pr]() {
    id = std::this_thread::get_id();
    pr.SetValue();
  }).join();
}

// Test Then inside another thread.
TEST(Future, ContinuationOnCurrent2) {
  Promise<> pr;
  auto ft = pr.GetFuture();
  pr.SetValue();
  std::thread([&ft]() {
    auto id = std::this_thread::get_id();
    ft.Then([id]() {
      EXPECT_EQ(std::this_thread::get_id(), id);
    });
  }).join();
}

}  // namespace trpc
