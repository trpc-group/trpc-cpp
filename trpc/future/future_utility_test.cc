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

#include "trpc/future/future_utility.h"

#include <memory>
#include <string>
#include <thread>
#include <tuple>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/future/future.h"

namespace trpc {

// Subclass of ExceptionBase.
class TestWhenAllException : public ExceptionBase {
 public:
  const char* what() const noexcept { return "test when all"; }
};

// Test empty future to WhenALl.
TEST(WhenAll, TestEmptyInputFuture) {
  auto flag = std::make_shared<bool>(false);
  WhenAll().Then([flag](std::tuple<>&& result) {
    *flag = true;
    return MakeReadyFuture<>();
  });

  EXPECT_TRUE(*flag);
}

// Test single future to WhenAll.
TEST(WhenAll, TestSingleInputFuture) {
  auto flag = std::make_shared<bool>(false);
  auto fut = MakeReadyFuture<uint32_t>(20190001);

  WhenAll(std::move(fut)).Then([flag](std::tuple<Future<uint32_t>>&& result) {
    auto fut = std::move(std::get<0>(result));
    EXPECT_TRUE(fut.IsReady());
    EXPECT_TRUE(fut.GetValue0() == 20190001);
    *flag = true;
    return MakeReadyFuture<>();
  });

  EXPECT_TRUE(*flag);
}

// Test triple future to WhenAll.
TEST(WhenAll, TestTripleInputFutures) {
  auto flag = std::make_shared<bool>(false);
  auto fut1 = MakeReadyFuture<uint32_t>(20190001);
  auto fut2 = MakeReadyFuture<std::string>("hello");
  auto fut3 = MakeReadyFuture<>();

  WhenAll(std::move(fut1), std::move(fut2), std::move(fut3))
    .Then([flag](std::tuple<Future<uint32_t>, Future<std::string>, Future<>>&& result) {
      auto fut1 = std::move(std::get<0>(result));
      EXPECT_TRUE(fut1.IsReady());
      EXPECT_TRUE(fut1.GetValue0() == 20190001);

      auto fut2 = std::move(std::get<1>(result));
      EXPECT_TRUE(fut2.IsReady());
      EXPECT_TRUE(fut2.GetValue0() == "hello");

      auto fut3 = std::move(std::get<2>(result));
      EXPECT_TRUE(fut3.IsReady());
      *flag = true;
      return MakeReadyFuture<>();
    });

  EXPECT_TRUE(*flag);
}

// Test triple unready future to WhenAll.
TEST(WhenAll, TestTripleInputUnreadyFutures) {
  auto flag = std::make_shared<bool>(false);
  auto pr1 = Promise<uint32_t>();
  auto pr2 = Promise<std::string>();
  auto pr3 = Promise<>();
  auto fut1 = pr1.GetFuture();
  auto fut2 = pr2.GetFuture();
  auto fut3 = pr3.GetFuture();

  WhenAll(std::move(fut1), std::move(fut2), std::move(fut3))
    .Then([flag](std::tuple<Future<uint32_t>, Future<std::string>, Future<>>&& result) {
      auto fut1 = std::move(std::get<0>(result));
      EXPECT_TRUE(fut1.IsReady());
      EXPECT_TRUE(fut1.GetValue0() == 20190911);

      auto fut2 = std::move(std::get<1>(result));
      EXPECT_TRUE(fut2.IsReady());
      EXPECT_TRUE(fut2.GetValue0() == "hello");

      auto fut3 = std::move(std::get<2>(result));
      EXPECT_TRUE(fut3.IsReady());
      *flag = true;
      return MakeReadyFuture<>();
    });

  EXPECT_FALSE(*flag);
  pr2.SetValue(std::string("hello"));
  EXPECT_FALSE(*flag);
  pr3.SetValue();
  EXPECT_FALSE(*flag);
  pr1.SetValue(20190911);

  EXPECT_TRUE(*flag);
}

// Test multiple unready future to WhenAll with disorder ready.
TEST(WhenAll, TestFuturesOrder) {
  auto flag = std::make_shared<bool>(false);
  auto pr1 = Promise<uint32_t>();
  auto pr2 = Promise<uint32_t>();
  auto pr3 = Promise<uint32_t>();
  auto fut1 = pr1.GetFuture();
  auto fut2 = pr2.GetFuture();
  auto fut3 = pr3.GetFuture();

  WhenAll(std::move(fut1), std::move(fut2), std::move(fut3))
    .Then([flag](std::tuple<Future<uint32_t>, Future<uint32_t>, Future<uint32_t>>&& result) {
      auto fut1 = std::move(std::get<0>(result));
      EXPECT_TRUE(fut1.IsReady());
      EXPECT_TRUE(fut1.GetValue0() == 1);

      auto fut2 = std::move(std::get<1>(result));
      EXPECT_TRUE(fut2.IsReady());
      EXPECT_TRUE(fut2.GetValue0() == 2);

      auto fut3 = std::move(std::get<2>(result));
      EXPECT_TRUE(fut3.IsReady());
      EXPECT_TRUE(fut3.GetValue0() == 3);
      *flag = true;
      return MakeReadyFuture<>();
    });

  EXPECT_FALSE(*flag);
  pr2.SetValue(2);
  EXPECT_FALSE(*flag);
  pr3.SetValue(3);
  EXPECT_FALSE(*flag);
  pr1.SetValue(1);

  EXPECT_TRUE(*flag);
}

// Test input future exceptional to WhenAll.
TEST(WhenAll, TestAnyFutureException) {
  auto flag = std::make_shared<bool>(false);
  auto pr1 = Promise<uint32_t>();
  auto pr2 = Promise<uint32_t>();
  auto pr3 = Promise<uint32_t>();
  auto fut1 = pr1.GetFuture();
  auto fut2 = pr2.GetFuture();
  auto fut3 = pr3.GetFuture();

  WhenAll(std::move(fut1), std::move(fut2), std::move(fut3))
    .Then([flag](std::tuple<Future<uint32_t>, Future<uint32_t>, Future<uint32_t>>&& result) {
      auto fut1 = std::move(std::get<0>(result));
      EXPECT_TRUE(fut1.IsReady());
      EXPECT_TRUE(fut1.GetValue0() == 1);

      auto fut2 = std::move(std::get<1>(result));
      EXPECT_TRUE(fut2.IsFailed());
      EXPECT_TRUE(fut2.GetException().is(TestWhenAllException()));

      auto fut3 = std::move(std::get<2>(result));
      EXPECT_TRUE(fut3.IsReady());
      EXPECT_TRUE(fut3.GetValue0() == 3);
      *flag = true;
      return MakeReadyFuture<>();
    });

  EXPECT_FALSE(*flag);
  pr2.SetException(TestWhenAllException());
  EXPECT_FALSE(*flag);
  pr3.SetValue(3);
  EXPECT_FALSE(*flag);
  pr1.SetValue(1);

  EXPECT_TRUE(*flag);
}

// Test vector with future member to WhenALl.
TEST(WhenAll, TestFuturesOrderVector) {
  auto flag = std::make_shared<bool>(false);
  auto pr1 = Promise<uint32_t>();
  auto pr2 = Promise<uint32_t>();
  auto pr3 = Promise<uint32_t>();
  std::vector<Future<uint32_t>> input;
  input.push_back(pr1.GetFuture());
  input.push_back(pr2.GetFuture());
  input.push_back(pr3.GetFuture());

  WhenAll(input.begin(), input.end()).Then([flag](std::vector<Future<uint32_t>>&& result) {
    for (uint32_t i = 0; i < 3; i++) {
      EXPECT_TRUE(result[i].IsReady());
      EXPECT_TRUE(result[i].GetValue0() == i + 1);
    }
    *flag = true;
    return MakeReadyFuture<>();
  });

  EXPECT_FALSE(*flag);
  pr2.SetValue(2);
  EXPECT_FALSE(*flag);
  pr3.SetValue(3);
  EXPECT_FALSE(*flag);
  pr1.SetValue(1);

  EXPECT_TRUE(*flag);
}

// Test vector with exceptional future to WhenALl.
TEST(WhenAll, TestAnyFutureExceptionVector) {
  auto flag = std::make_shared<bool>(false);
  auto pr1 = Promise<uint32_t>();
  auto pr2 = Promise<uint32_t>();
  auto pr3 = Promise<uint32_t>();
  std::vector<Future<uint32_t>> input;
  input.push_back(pr1.GetFuture());
  input.push_back(pr2.GetFuture());
  input.push_back(pr3.GetFuture());

  WhenAll(input.begin(), input.end()).Then([flag](std::vector<Future<uint32_t>>&& result) {
    auto fut1 = std::move(result[0]);
    EXPECT_TRUE(fut1.IsReady());
    EXPECT_TRUE(fut1.GetValue0() == 1);

    auto fut2 = std::move(result[1]);
    EXPECT_TRUE(fut2.IsFailed());
    EXPECT_TRUE(fut2.GetException().is(TestWhenAllException()));

    auto fut3 = std::move(result[2]);
    EXPECT_TRUE(fut3.IsReady());
    EXPECT_TRUE(fut3.GetValue0() == 3);
    *flag = true;
    return MakeReadyFuture<>();
  });
  EXPECT_FALSE(*flag);
  pr2.SetException(TestWhenAllException());
  EXPECT_FALSE(*flag);
  pr3.SetValue(3);
  EXPECT_FALSE(*flag);
  pr1.SetValue(1);

  EXPECT_TRUE(*flag);
}

// Test DoUntil with stop function return true.
TEST(DoUntil, TestLoop) {
  auto flag = std::make_shared<bool>(false);
  auto reach = std::make_shared<bool>(false);
  auto counter = std::make_shared<uint32_t>(0);

  DoUntil(
    [flag]() { return *flag; },
    [flag, counter]() {
      (*counter)++;
      if (*counter == 12) {
        *flag = true;
      }
      return MakeReadyFuture<>();
    }).Then([reach, counter]() {
      *reach = true;
      EXPECT_TRUE(*counter == 12);
      return MakeReadyFuture<>();
    });

  EXPECT_TRUE(*flag);
  EXPECT_TRUE(*reach);
  EXPECT_TRUE(*counter == 12);
}

// Test DoUntil with callback return exception future.
TEST(DoUntil, TestExceptionLoop) {
  auto flag = std::make_shared<bool>(false);
  auto reach = std::make_shared<bool>(false);
  auto counter = std::make_shared<uint32_t>(false);

  DoUntil(
    [flag]() { return *flag; },
    [flag, counter]() {
      (*counter)++;
      if (*counter == 12) {
        *flag = true;
      }
      if (*counter == 7) {
        return MakeExceptionFuture<>(TestWhenAllException());
      }
      return MakeReadyFuture<>();
    }).Then([reach]() {
      ADD_FAILURE();
      return MakeReadyFuture<>();
    });

  EXPECT_FALSE(*flag);
  EXPECT_FALSE(*reach);
  EXPECT_TRUE(*counter == 7);
}

// Test DoUntil finished by user callback return ready future with false value.
TEST(DoUntil, TestUserCallback) {
  int counter = 0;
  int sum = 0;
  EXPECT_TRUE(
    DoUntil([&counter, &sum](){
      if (counter++ == 10) return MakeReadyFuture<bool>(false);
      ++sum;
      return MakeReadyFuture<bool>(true);
    }).IsReady());
  EXPECT_EQ(sum, 10);
}

// Test empty vector to WhenAny.
TEST(WhenAny, test_empty_input_future) {
  auto flag = std::make_shared<bool>(false);

  std::vector<Future<>> vecs;
  WhenAny(vecs.begin(), vecs.end()).Then([flag](Future<size_t, std::tuple<>>&& fut) {
    *flag = true;
    return MakeReadyFuture<>();
  });

  EXPECT_TRUE(*flag);
}

// Test ready future to WhenAny.
TEST(WhenAny, TestReadyFuture) {
  std::vector<Future<bool>> vecs;
  Promise<bool> promise_0;
  Promise<bool> promise_1;
  vecs.push_back(promise_0.GetFuture());
  vecs.push_back(promise_1.GetFuture());
  auto flag = std::make_shared<bool>(false);
  size_t index = 0;

  WhenAny(vecs.begin(), vecs.end()).Then([flag, &index](Future<size_t, std::tuple<bool>>&& fut) {
    EXPECT_TRUE(fut.IsReady());
    index = std::get<0>(fut.GetValue());
    *flag = true;
    return MakeReadyFuture<>();
  });

  EXPECT_EQ(index, 0);
  promise_1.SetValue(true);
  EXPECT_EQ(index, 1);
  EXPECT_TRUE(*flag);
}

// Test ready future with multiple parameter to WhenAny.
TEST(WhenAny, TestReadyFutureWithMultiParam) {
  std::vector<Future<bool, int>> vecs;
  Promise<bool, int> promise_0;
  Promise<bool, int> promise_1;
  vecs.push_back(promise_0.GetFuture());
  vecs.push_back(promise_1.GetFuture());
  auto flag = std::make_shared<bool>(false);
  size_t index = 0;

  WhenAny(vecs.begin(), vecs.end())
    .Then([flag, &index](Future<size_t, std::tuple<bool, int>>&& fut) {
      EXPECT_TRUE(fut.IsReady());
      auto value = fut.GetValue();
      index = std::get<0>(value);
      auto tuple_value = std::get<1>(value);
      EXPECT_EQ(true, std::get<0>(tuple_value));
      EXPECT_EQ(1, std::get<1>(tuple_value));

      *flag = true;
      return MakeReadyFuture<>();
    });

  EXPECT_EQ(index, 0);
  promise_1.SetValue(true, 1);
  EXPECT_EQ(index, 1);
  EXPECT_TRUE(*flag);
}

// Test exceptional future to WhenAny.
TEST(WhenAny, TestExceptionFuture) {
  std::vector<Future<bool>> vecs;
  Promise<bool> promise_0;
  Promise<bool> promise_1;
  vecs.push_back(promise_0.GetFuture());
  vecs.push_back(promise_1.GetFuture());
  auto flag = std::make_shared<bool>(false);

  WhenAny(vecs.begin(), vecs.end()).Then([flag](Future<size_t, std::tuple<bool>>&& fut) {
    EXPECT_TRUE(fut.IsFailed());
    *flag = true;
    return MakeReadyFuture<>();
  });

  EXPECT_FALSE(*flag);
  promise_1.SetException(CommonException("error"));
  EXPECT_TRUE(*flag);
}

// Test empty vector to WhenAnyWithoutException.
TEST(WhenAnyWithoutException, TestEmptyInputFuture) {
  auto flag = std::make_shared<bool>(false);
  std::vector<Future<>> vecs;

  WhenAnyWithoutException(vecs.begin(), vecs.end())
    .Then([flag](Future<size_t, std::tuple<>>&& fut) {
      *flag = true;
      return MakeReadyFuture<>();
    });

  EXPECT_TRUE(*flag);
}

// Test ready future to WhenAnyWithoutException.
TEST(WhenAnyWithoutException, TestReadyFuture) {
  std::vector<Future<bool>> vecs;
  Promise<bool> promise_0;
  Promise<bool> promise_1;
  vecs.push_back(promise_0.GetFuture());
  vecs.push_back(promise_1.GetFuture());
  auto flag = std::make_shared<bool>(false);
  size_t index = 0;

  WhenAnyWithoutException(vecs.begin(), vecs.end())
    .Then([flag, &index](Future<size_t, std::tuple<bool>>&& fut) {
      EXPECT_TRUE(fut.IsReady());
      index = std::get<0>(fut.GetValue());
      *flag = true;
      return MakeReadyFuture<>();
    });

  EXPECT_EQ(index, 0);
  promise_1.SetValue(true);
  EXPECT_EQ(index, 1);
  EXPECT_TRUE(*flag);
}

// Test ready future with multiple parameter to WhenAnyWithoutException.
TEST(WhenAnyWithoutException, TestReadyFutureWithMultiParam) {
  std::vector<Future<bool, int>> vecs;
  Promise<bool, int> promise_0;
  Promise<bool, int> promise_1;
  vecs.push_back(promise_0.GetFuture());
  vecs.push_back(promise_1.GetFuture());
  auto flag = std::make_shared<bool>(false);
  size_t index = 0;

  WhenAnyWithoutException(vecs.begin(), vecs.end())
    .Then([flag, &index](Future<size_t, std::tuple<bool, int>>&& fut) {
      EXPECT_TRUE(fut.IsReady());
      auto value = fut.GetValue();
      index = std::get<0>(value);
      auto tuple_value = std::get<1>(value);
      EXPECT_EQ(true, std::get<0>(tuple_value));
      EXPECT_EQ(1, std::get<1>(tuple_value));

      *flag = true;
      return MakeReadyFuture<>();
    });

  EXPECT_EQ(index, 0);
  promise_1.SetValue(true, 1);
  EXPECT_EQ(index, 1);
  EXPECT_TRUE(*flag);
}

// Test exceptional and ready future to WhenAnyWithoutException.
TEST(WhenAnyWithoutException, TestExceptionReadyFuture) {
  std::vector<Future<bool>> vecs;
  Promise<bool> promise_0;
  Promise<bool> promise_1;
  Promise<bool> promise_2;
  vecs.push_back(promise_0.GetFuture());
  vecs.push_back(promise_1.GetFuture());
  vecs.push_back(promise_2.GetFuture());
  auto flag = std::make_shared<bool>(false);
  size_t index = 0;

  WhenAnyWithoutException(vecs.begin(), vecs.end())
    .Then([flag, &index](Future<size_t, std::tuple<bool>>&& fut) {
      EXPECT_TRUE(fut.IsReady());
      index = std::get<0>(fut.GetValue());
      *flag = true;
      return MakeReadyFuture<>();
    });

  EXPECT_EQ(index, 0);
  promise_1.SetException(CommonException("error"));
  EXPECT_EQ(index, 0);
  EXPECT_FALSE(*flag);
  promise_2.SetValue(true);
  EXPECT_EQ(index, 2);
  EXPECT_TRUE(*flag);
}

// Test all exceptional future to WhenAnyWithoutException.
TEST(WhenAnyWithoutException, TestExceptionFuture) {
  std::vector<Future<bool>> vecs;
  Promise<bool> promise_0;
  Promise<bool> promise_1;
  vecs.push_back(promise_0.GetFuture());
  vecs.push_back(promise_1.GetFuture());
  auto flag = std::make_shared<bool>(false);
  size_t index = 0;

  WhenAnyWithoutException(vecs.begin(), vecs.end())
    .Then([flag, &index](Future<size_t, std::tuple<bool>>&& fut) {
      EXPECT_TRUE(fut.IsFailed());
      *flag = true;
      return MakeReadyFuture<>();
    });

  EXPECT_EQ(index, 0);
  promise_1.SetException(CommonException("error"));
  EXPECT_EQ(index, 0);
  EXPECT_FALSE(*flag);
  promise_0.SetException(CommonException("error"));
  EXPECT_TRUE(*flag);
}

TEST(DoWhile, Test) {
  int counter = 0;
  int sum = 0;
  EXPECT_TRUE(
    DoWhile([&counter]() { return counter++ < 10; },
            [&sum]() {
              ++sum;
              return MakeReadyFuture<>();
            }).IsReady());
  EXPECT_EQ(sum, 10);
}

TEST(Repeat, Test) {
  int counter = 0;
  EXPECT_TRUE(
    Repeat([&counter]() {
            if (++counter == 10)
              return MakeExceptionFuture<>(CommonException("10"));
            else
              return MakeReadyFuture<>();
          }).IsFailed());
}

TEST(DoForEach, Test) {
  std::vector<int> c(10, 1);
  int counter = 0;

  EXPECT_TRUE(DoForEach(c.begin(), c.end(), [&counter]() {
                ++counter;
                return MakeReadyFuture<>();
              }).IsReady());

  EXPECT_TRUE(DoForEach(c.begin(), c.end(), [&counter](auto e) {
                counter -= e;
                return MakeReadyFuture<>();
              }).IsReady());

  EXPECT_EQ(counter, 0);

  EXPECT_TRUE(DoForEach(c, [&counter](auto e) {
                counter += e;
                return MakeReadyFuture<>();
              }).IsReady());

  EXPECT_EQ(counter, c.size());
}

TEST(DoFor, Test) {
  int counter = 0;
  EXPECT_TRUE(DoFor(10, [&counter]() {
                ++counter;
                return MakeReadyFuture<>();
              }).IsReady());
  EXPECT_EQ(counter, 10);

  EXPECT_TRUE(DoFor(counter, [&counter](std::size_t i) {
                EXPECT_EQ(i + counter, 10);
                --counter;
                return MakeReadyFuture<>();
              }).IsReady());

  EXPECT_EQ(counter, 0);
}

TEST(BlockingGet, test) {
  Promise<> promise;
  auto fut = promise.GetFuture();
  auto t = std::thread([promise = std::move(promise)]() mutable {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    promise.SetValue();
  });
  t.detach();
  auto res_fut = future::BlockingGet(std::move(fut));
  ASSERT_TRUE(res_fut.IsReady() || res_fut.IsFailed());
}

bool TestBlockingTryGet(uint32_t execute_time_ms, uint32_t timeout_ms) {
  Promise<> promise;
  auto fut = promise.GetFuture();
  auto t = std::thread([promise = std::move(promise), execute_time_ms]() mutable {
    std::this_thread::sleep_for(std::chrono::milliseconds(execute_time_ms));
    promise.SetValue();
  });
  t.detach();
  auto res_fut = future::BlockingTryGet(std::move(fut), timeout_ms);
  if (res_fut.has_value()) {
    EXPECT_TRUE(res_fut->IsReady() || res_fut->IsFailed());
    return true;
  }

  return false;
}

TEST(BlockingTryGet, test) {
  ASSERT_FALSE(TestBlockingTryGet(1000, 100));
  ASSERT_TRUE(TestBlockingTryGet(100, 1000));
}

// Test in the capture promise by reference situation, the promise object is not destroyed during SetValue
TEST(BlockingGet, capture_promise_by_ref) {
  constexpr int kMaxLoopTimes = 10000;
  std::atomic<int> execute_times = 0;
  for (int i = 0; i < kMaxLoopTimes; ++i) {
    std::unique_ptr<std::thread> t;
    {
      trpc::Promise<> pr;
      auto fut = pr.GetFuture();
      t = std::make_unique<std::thread>([&]() {
        pr.SetValue();
        execute_times++;
      });
      future::BlockingGet(std::move(fut));
    }
    t->join();
  }
  EXPECT_EQ(execute_times.load(), kMaxLoopTimes);
}

}  // namespace trpc
