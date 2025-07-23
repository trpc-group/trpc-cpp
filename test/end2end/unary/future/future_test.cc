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

#include <chrono>
#include <string>
#include <thread>

#include "test/end2end/unary/future/future_fixture.h"
#include "trpc/client/client_context.h"
#include "trpc/client/make_client_context.h"
#include "trpc/common/future/future_utility.h"
#include "trpc/runtime/separate_runtime.h"
#include "trpc/util/thread/latch.h"

namespace trpc::testing {

namespace {

using TestRequest = trpc::testing::future::TestRequest;
using TestReply = trpc::testing::future::TestReply;
using FutureServiceProxy = TestFuture::FutureServiceProxy;

// Request server in synchronic mode, maybe success or failed.
// Will block current thread.
void SyncCall(std::shared_ptr<FutureServiceProxy> proxy, bool expected) {
  TestRequest req;
  TestReply rsp;
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  trpc::Status status = proxy->SayHello(ctx, req, &rsp);
  ASSERT_EQ(status.OK(), expected);
}

// Request server in synchronic mode, will failed due to response timeout.
// Will block current thread.
void SyncTimeoutCall(std::shared_ptr<FutureServiceProxy> proxy) {
  TestRequest req;
  TestReply rsp;
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  // Server will delay response to trigger timeout for client.
  trpc::Status status = proxy->SayHelloTimeout(ctx, req, &rsp);
  ASSERT_FALSE(status.OK());
}

// Request server in asynchronous mode, maybe success or failed.
// Will NOT block current thread.
trpc::Future<bool> AsyncCall(std::shared_ptr<FutureServiceProxy> proxy) {
  TestRequest req;
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  auto fut = proxy->AsyncSayHello(ctx, req)
                   .Then([](trpc::Future<TestReply>&& fut) {
                     return trpc::MakeReadyFuture<bool>(fut.IsReady());
                   });
  return fut;
}

// Request server in asynchronous mode, returned future will finally become exceptional.
// Will NOT block current thread.
trpc::Future<bool> AsyncTimeoutCall(std::shared_ptr<FutureServiceProxy> proxy) {
  TestRequest req;
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  auto fut = proxy->AsyncSayHelloTimeout(ctx, req)
                   .Then([](trpc::Future<TestReply>&& fut) {
                     return trpc::MakeReadyFuture<bool>(fut.IsFailed());
                   });
  return fut;
}

// Send request to server, will not recieve response.
void SendOnlyCall(std::shared_ptr<FutureServiceProxy> proxy) {
  TestRequest req;
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  trpc::Status status = proxy->SayHello(ctx, req);
  ASSERT_TRUE(status.OK());
}

// Called in main thread, and use returned future to register callback, which will be
// run in frame thread.
trpc::Future<> SwitchIntoFrameThread() {
  TestRequest req;
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(TestFuture::tcp_complex_proxy_);
  auto fut = TestFuture::tcp_complex_proxy_->AsyncSayHello(ctx, req)
                                            .Then([](trpc::Future<TestReply>&& fut) {
                                              return trpc::MakeReadyFuture<>();
                                            });
  return fut;
}

#ifdef TRPC_SEPARATE_RUNTIME_TEST
// Execute task in handle thread of separate thread model.
// Note: this function should be called outside handle thread.
void ExecuteTaskInHandleThread(::trpc::Function<void()>&& task_handler) {
  ::trpc::Latch l(1);
  auto* thread_model = trpc::separate::RandomGetSeparateThreadModel();
  auto* task = trpc::object_pool::New<trpc::MsgTask>();
  task->handler = [&l, task_handler = std::move(task_handler)]() {
    task_handler();
    l.count_down();
  };
  bool ret = trpc::separate::SubmitHandleTask(thread_model, task);
  ASSERT_TRUE(ret);
  l.wait();
}
#endif

}  // namespace

// Future base.
// Summary: Call BlockingTryGet function on a ready future which has empty template parameter.
// Expected: True
TEST(EmptyTemplateParameter, ReadyFutureWait) {
  trpc::Future<> fut = trpc::MakeReadyFuture<>();
  ASSERT_TRUE(trpc::future::BlockingGet(std::move(fut)).IsReady());
}

// Summary: Set promise in another thread, in which trigger the callback.
// Expected: Callback executed thread is not current thread.
TEST(EmptyTemplateParameter, CrossThread) {
  std::thread::id caller;
  trpc::Promise<> pr;
  trpc::Future<> fut = pr.GetFuture();
  std::thread th([p = std::move(pr)]() mutable {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                p.SetValue();
              });
  th.detach();
  auto ft = fut.Then([&caller]() {
    caller = std::this_thread::get_id();
    return trpc::MakeReadyFuture<>();
  });
  trpc::future::BlockingGet(std::move(ft));
  ASSERT_NE(std::this_thread::get_id(), caller);
}

// Summary: Call BlockingTryGet function on unready future with timeout expired set.
// Expected: False.
TEST(EmptyTemplateParameter, UnreadyFutureWaitTimeout) {
  trpc::Promise<> pr;
  trpc::Future<> fut = pr.GetFuture();
  ASSERT_FALSE(trpc::future::BlockingTryGet(std::move(fut), 10).has_value());
}

// Summary: SetValue multiple times, see how many times the callback executed.
// Expected: Callback executed only once.
TEST(EmptyTemplateParameter, CallbackRunOnlyOnce) {
  int count = 0;
  trpc::Promise<> pr;
  trpc::Future<> fut = pr.GetFuture();
  fut.Then([&count]() {
        count++;
      });
  ASSERT_EQ(count, 0);
  pr.SetValue();
  ASSERT_EQ(count, 1);
  pr.SetValue();
  ASSERT_EQ(count, 1);
}

// Summary: SetValue after SetException, to see the state of future.
// Expected: Future is ready, not exceptional.
TEST(EmptyTemplateParameter, ReadyCoverException) {
  bool flag = false;
  trpc::Promise<> pr;
  trpc::Future<> fut = pr.GetFuture();
  pr.SetException(CommonException("Test"));
  pr.SetValue();
  fut.Then([&flag]() {
        flag = true;
      });
  ASSERT_TRUE(flag);
}

// Summary: Call GetValue on ready future, which has single template parameter.
// Expected: Got initial value by first tuple element.
TEST(SingleTemplateParameter, GetValue) {
  trpc::Future<int> fut = trpc::MakeReadyFuture<int>(10);
  ASSERT_EQ(std::get<0>(fut.GetValue()), 10);
}

// Summary: Same as EmptyTemplateParameter, but future has single template parameter.
// Expected: Callback executed thread is not current thread.
TEST(SingleTemplateParameter, CrossThread) {
  std::thread::id caller;
  trpc::Promise<int> pr;
  trpc::Future<int> fut = pr.GetFuture();
  std::thread th([p = std::move(pr)]() mutable {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                p.SetValue(10);
              });
  th.detach();
  auto ft = fut.Then([&caller](int&& val) {
    caller = std::this_thread::get_id();
    return trpc::MakeReadyFuture<>();
  });
  trpc::future::BlockingGet(std::move(ft));
  ASSERT_NE(std::this_thread::get_id(), caller);
}

// Summary: Same as EmptyTemplateParameter.
// Expected: Callback executed only once.
TEST(SingleTemplateParameter, CallbackRunOnlyOnce) {
  int count = 0;
  trpc::Promise<int> pr;
  trpc::Future<int> fut = pr.GetFuture();
  fut.Then([&count](int&& val) {
        count++;
      });
  ASSERT_EQ(count, 0);
  pr.SetValue(10);
  ASSERT_EQ(count, 1);
  pr.SetValue(20);
  ASSERT_EQ(count, 1);
}

// Summary: SetValue after SetException, to see the state of future.
// Expected: Future is ready.
TEST(SingleTemplateParameter, ReadyCoverException) {
  int val = 0;
  trpc::Promise<int> pr;
  trpc::Future<int> fut = pr.GetFuture();
  pr.SetException(CommonException("Test"));
  pr.SetValue(20);
  fut.Then([&val](int&& num) {
        val = num;
      });
  ASSERT_EQ(val, 20);
}

// Summary: Call IsReady on ready future, which has multiple template parameter.
// Expected: True.
TEST(MultipleTemplateParameter, IsReady) {
  trpc::Future<int, int> fut = trpc::MakeReadyFuture<int, int>(10, 20);
  ASSERT_TRUE(fut.IsReady());
}

// Summary: Register callback which return void to a ready future.
// Expected: Callback executed.
TEST(MultipleTemplateParameter, ReadyFutureTriggerCallbackWithoutReturn) {
  bool flag = false;
  trpc::Future<int, int> fut = trpc::MakeReadyFuture<int, int>(10, 20);
  fut.Then([&flag](int&& a, int&& b) {
        flag = true;
      });
  ASSERT_TRUE(flag);
}

// Summary: Register callback which return a future to a ready future.
// Expected: Callback executed.
TEST(MultipleTemplateParameter, ReadyFutureTriggerCallbackWithReturn) {
  bool flag = false;
  trpc::Future<int, int> fut = trpc::MakeReadyFuture<int, int>(10, 20);
  fut.Then([&flag](int&& a, int&& b) {
        flag = true;
        return trpc::MakeReadyFuture<>();
      });
  ASSERT_TRUE(flag);
}

// Summary: Call GetValue function on ready future, which has multiple template parameter.
// Expected: Got values one by one in tuple.
TEST(MultipleTemplateParameter, GetValue) {
  trpc::Future<int, int> fut = trpc::MakeReadyFuture<int, int>(10, 20);
  std::tuple<int, int> tup = fut.GetValue();
  ASSERT_EQ(std::get<0>(tup), 10);
  ASSERT_EQ(std::get<1>(tup), 20);
}

// Summary: Call BlockingTryGet on ready future, which has multiple template parameter.
// Expected: True.
TEST(MultipleTemplateParameter, ReadyFutureWait) {
  trpc::Future<int, int> fut = trpc::MakeReadyFuture<int, int>(10, 20);
  ASSERT_TRUE(trpc::future::BlockingGet(std::move(fut)).IsReady());
}

// Summary: Same as EmptyTemplateParameter.
// Expected: Callback executed thread is not current thread.
TEST(MultipleTemplateParameter, CrossThread) {
  std::thread::id caller;
  trpc::Promise<int, int> pr;
  trpc::Future<int, int> fut = pr.GetFuture();
  std::thread th([p = std::move(pr)]() mutable {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                p.SetValue(10, 20);
              });
  th.detach();
  auto ft = fut.Then([&caller](int&& a, int&& b) {
    caller = std::this_thread::get_id();
    return trpc::MakeReadyFuture<>();
  });
  trpc::future::BlockingGet(std::move(ft));
  ASSERT_NE(std::this_thread::get_id(), caller);
}

// Summary: Call IsFailed on exceptional future.
// Expected: True.
TEST(MultipleTemplateParameter, IsFailed) {
  trpc::Future<int, int> fut = trpc::MakeExceptionFuture<int, int>(CommonException("Test"));
  ASSERT_TRUE(fut.IsFailed());
}

// Summary: Register callback which has non-future parameter and return void to exceptional
//          future, to see callback triggered or not.
// Expected: Callback not triggered.
TEST(MultipleTemplateParameter, ExceptionFutureIgnoreCallbackWithoutReturn) {
  bool flag = false;
  trpc::Future<int, int> fut = trpc::MakeExceptionFuture<int, int>(CommonException("Test"));
  fut.Then([&flag](int&& a, int&& b) {
        flag = true;
      });
  ASSERT_FALSE(flag);
}

// Summary: Register callback which has non-future parameter and return future to exceptional
//          future, to see callback triggered or not.
// Expected: Callback not triggered.
TEST(MultipleTemplateParameter, ExceptionFutureIgnoreCallbackWithReturn) {
  bool flag = false;
  trpc::Future<int, int> fut = trpc::MakeExceptionFuture<int, int>(CommonException("Test"));
  fut.Then([&flag](int&& a, int&& b) {
        flag = true;
        return trpc::MakeReadyFuture<>();
      });
  ASSERT_FALSE(flag);
}

// Summary: SetValue multiple times on ready future, to see how many times callback triggered.
// Expected: Callback triggered only once.
TEST(MultipleTemplateParameter, CallbackRunOnlyOnce) {
  int count = 0;
  trpc::Promise<int, int> pr;
  trpc::Future<int, int> fut = pr.GetFuture();
  fut.Then([&count](int&& a, int&& b) {
        count++;
      });
  ASSERT_EQ(count, 0);
  pr.SetValue(10, 20);
  ASSERT_EQ(count, 1);
  pr.SetValue(20, 30);
  ASSERT_EQ(count, 1);
}

// Summary: SetValue after SetExcepton, to see final state of future.
// Expected: Future in ready state.
TEST(MultipleTemplateParameter, ReadyCoverException) {
  int val = 0;
  trpc::Promise<int, int> pr;
  trpc::Future<int, int> fut = pr.GetFuture();
  pr.SetException(CommonException("Test"));
  pr.SetValue(20, 30);
  fut.Then([&val](int&& a, int&& b) {
        val = a + b;
      });
  ASSERT_EQ(val, 50);
}

// Future utility.
TEST(DoUntil, TerminalRightNow) {
  int count = 0;
  trpc::Future<> fut = trpc::DoUntil(
                       []() {
                         return true;
                       },
                       [&count]() {
                         count++;
                         return trpc::MakeReadyFuture<>();
                       });
  ASSERT_EQ(count, 0);
  ASSERT_TRUE(fut.IsReady());
}

// Summary: DoUntil terminaled by stop function.
// Expected: Function executed 10 times.
TEST(DoUntil, TerminalByStop) {
  int count = 0;
  trpc::Future<> fut = trpc::DoUntil(
                       [&count]() {
                         return count >= 10;
                       },
                       [&count]() {
                         count++;
                         return trpc::MakeReadyFuture<>();
                       });
  ASSERT_EQ(count, 10);
  ASSERT_TRUE(fut.IsReady());
}

// Summary: DoUntil terminaled by function which finally return exceptional future.
// Expected: Function executed 5 times.
TEST(DoUntil, TerminalByFunc) {
  int count = 0;
  trpc::Future<> fut = trpc::DoUntil(
                       []() {
                         return false;
                       },
                       [&count]() {
                         count++;
                         if (count >= 5) return trpc::MakeExceptionFuture<>(CommonException("Test"));
                         return trpc::MakeReadyFuture<>();
                       });
  ASSERT_EQ(count, 5);
  ASSERT_TRUE(fut.IsFailed());
}

// Summary: DoUntil without stop function, and terminaled by callback
//          return ready future with value false.
// Expected: Function run 10 times.
TEST(DoUntil, TerminalWithoutStopByFalse) {
  int count = 0;
  trpc::Future<> fut = trpc::DoUntil(
                       [&count]() {
                         count++;
                         if (count >= 10) return trpc::MakeReadyFuture<bool>(false);
                         return trpc::MakeReadyFuture<bool>(true);
                       });
  ASSERT_EQ(count, 10);
  ASSERT_TRUE(fut.IsReady());
}

// Summary: DoUntil without stop function, and terminaled by callback
//          return exceptional future.
// Expected: Function run 10 times.
TEST(DoUntil, TerminalWithoutStopByException) {
  int count = 0;
  trpc::Future<> fut = trpc::DoUntil(
                       [&count]() {
                         count++;
                         if (count >= 10) return trpc::MakeExceptionFuture<bool>(CommonException("Test"));
                         return trpc::MakeReadyFuture<bool>(true);
                       });
  ASSERT_EQ(count, 10);
  ASSERT_TRUE(fut.IsFailed());
}

// Summary: DoWhile terminaled by stop function.
// Expected: Function run 10 times.
TEST(DoWhile, TerminalByStop) {
  int count = 0;
  trpc::Future<> fut = trpc::DoWhile(
                       [&count]() {
                         return count < 10;
                       },
                       [&count]() {
                         count++;
                         return trpc::MakeReadyFuture<>();
                       });
  ASSERT_EQ(count, 10);
  ASSERT_TRUE(fut.IsReady());
}

// Summary: Repeat terminaled by function return exceptional future.
// Expected: Function run 10 times.
TEST(Repeat, TerminalByException) {
  int count = 0;
  trpc::Future<> fut = trpc::Repeat(
                       [&count]() {
                         count++;
                         if (count >= 10) return trpc::MakeExceptionFuture<>(CommonException("Test"));
                         return trpc::MakeReadyFuture<>();
                       });
  ASSERT_EQ(count, 10);
  ASSERT_TRUE(fut.IsFailed());
}

// Summary: DoForEach callback funtion with empty parameter.
// Expected: Function run 10 times by vector size.
TEST(DoForEach, IterateWithoutParameter) {
  int count = 0;
  std::vector<int> vec(10, 1);
  trpc::DoForEach(vec.begin(),
                  vec.end(),
                  [&count]() {
                    count++;
                    return trpc::MakeReadyFuture<>();
                  });
  ASSERT_EQ(count, 10);
}

// Summary: DoForEach callback funtion with one parameter.
// Expected: Function run 10 times by vector size.
TEST(DoForEach, IterateWithParameter) {
  int count = 0;
  std::vector<int> vec(10, 1);
  trpc::DoForEach(vec.begin(),
                  vec.end(),
                  [&count](int num) {
                    count += num;
                    return trpc::MakeReadyFuture<>();
                  });
  ASSERT_EQ(count, 10);
}

// Summary: DoForEach with first parameter as container, and callback with one parameter.
// Expected: Function run 10 times by vector size.
TEST(DoForEach, ContainerWithParameter) {
  int count = 0;
  std::vector<int> vec(10, 1);
  trpc::DoForEach(vec,
                  [&count](int num) {
                    count += num;
                    return trpc::MakeReadyFuture<>();
                  });
  ASSERT_EQ(count, 10);
}

// Summary: DoFor callback without parameter.
// Expected: Callback run 10 times by first parameter.
TEST(DoFor, WithoutParameter) {
  int count = 0;
  trpc::DoFor(10,
              [&count]() {
                count++;
                return trpc::MakeReadyFuture<>();
              });
  ASSERT_EQ(count, 10);
}

// Summary: DoFor callback with parameter.
// Expected: Callback parameter from 0 to 9.
TEST(DoFor, WithParameter) {
  int count = 0;
  trpc::DoFor(10,
              [&count](std::size_t num) {
                count += num;
                return trpc::MakeReadyFuture<>();
              });
  ASSERT_EQ(count, 45);
}

// Summary: WhenAll callback without parameter.
// Expected: Returned future is ready.
TEST(WhenAll, WithoutParameter) {
  bool flag = false;
  trpc::WhenAll()
       .Then([&flag](std::tuple<>&& result) {
          flag = true;
        });
  ASSERT_TRUE(flag);
}

// Summary: WhenAll with single ready future as input.
// Expected: Callback executed, and got ready value inside callback.
TEST(WhenAll, WithSingleReadyFuture) {
  bool flag = false;
  trpc::Future<int> fut = trpc::MakeReadyFuture<int>(12345);
  trpc::WhenAll(std::move(fut))
       .Then([&flag](std::tuple<trpc::Future<int>>&& result) {
          flag = true;
          trpc::Future<int> fut = std::move(std::get<0>(result));
          EXPECT_EQ(fut.GetValue0(), 12345);
          return trpc::MakeReadyFuture<>();
        });
  ASSERT_TRUE(flag);
}

// Summary: WhenAll with three ready futures as input.
// Expected: Got ready futures one by one inside callback.
TEST(WhenAll, WithMultipleReadyFuture) {
  bool flag = false;
  trpc::Future<> fut_a = trpc::MakeReadyFuture<>();
  trpc::Future<int> fut_b = trpc::MakeReadyFuture<int>(12345);
  trpc::Future<std::string> fut_c = trpc::MakeReadyFuture<std::string>("Test");

  trpc::WhenAll(std::move(fut_a), std::move(fut_b), std::move(fut_c))
       .Then([&flag](std::tuple<trpc::Future<>, trpc::Future<int>, trpc::Future<std::string>>&& result) {
          flag = true;
          trpc::Future<int> fut_int = std::move(std::get<1>(result));
          EXPECT_EQ(fut_int.GetValue0(), 12345);
          trpc::Future<std::string> fut_str = std::move(std::get<2>(result));
          EXPECT_EQ(fut_str.GetValue0(), "Test");
          return trpc::MakeReadyFuture<>();
        });
  ASSERT_TRUE(flag);
}

// Summary: WhenAll with single unready future as input, after
//          which SetValue to trigger callback.
// Expected: Callback triggered after SetValue.
TEST(WhenAll, WithSingleUnreadyFuture) {
  bool flag = false;
  trpc::Promise<> pr;
  trpc::Future<> fut = pr.GetFuture();
  trpc::WhenAll(std::move(fut))
       .Then([&flag](std::tuple<trpc::Future<>>&& result) {
          flag = true;
          return trpc::MakeReadyFuture<>();
        });
  ASSERT_FALSE(flag);
  pr.SetValue();
  ASSERT_TRUE(flag);
}

// Summary: WhenAll with three unready futures as input, then SetValue/SetException one by one.
// Expected: Callback triggered after all futures turned resulted.
TEST(WhenAll, WithMultipleUnreadyFuture) {
  bool flag = false;
  trpc::Promise<> pr_a;
  trpc::Future<> fut_a = pr_a.GetFuture();
  trpc::Promise<int> pr_b;
  trpc::Future<int> fut_b = pr_b.GetFuture();
  trpc::Future<> fut_c = trpc::MakeReadyFuture<>();
  trpc::WhenAll(std::move(fut_a), std::move(fut_b), std::move(fut_c))
       .Then([&flag](std::tuple<trpc::Future<>, trpc::Future<int>, trpc::Future<>>&& result) {
          flag = true;
          trpc::Future<> fut_a = std::move(std::get<0>(result));
          EXPECT_TRUE(fut_a.IsFailed());
          trpc::Future<int> fut_b = std::move(std::get<1>(result));
          EXPECT_EQ(fut_b.GetValue0(), 123);
          return trpc::MakeReadyFuture<>();
        });
  ASSERT_FALSE(flag);
  pr_b.SetValue(123);
  ASSERT_FALSE(flag);
  pr_a.SetException(CommonException("Test"));
  ASSERT_TRUE(flag);
}

// Summary: WhenAll input iterator with member future ready.
// Expected: Got ready value one by one inside callback.
TEST(WhenAll, IterateWithReadyFuture) {
  bool flag = false;
  trpc::Future<int> fut_a = trpc::MakeReadyFuture<int>(10);
  trpc::Future<int> fut_b = trpc::MakeReadyFuture<int>(20);
  trpc::Future<int> fut_c = trpc::MakeReadyFuture<int>(30);
  std::vector<trpc::Future<int>> vec;
  vec.push_back(std::move(fut_a));
  vec.push_back(std::move(fut_b));
  vec.push_back(std::move(fut_c));
  trpc::WhenAll(vec.begin(), vec.end())
       .Then([&flag](std::vector<trpc::Future<int>>&& result) {
          flag = true;
          trpc::Future<int> fut_a = std::move(result[0]);
          EXPECT_EQ(fut_a.GetValue0(), 10);
          trpc::Future<int> fut_b = std::move(result[1]);
          EXPECT_EQ(fut_b.GetValue0(), 20);
          trpc::Future<int> fut_c = std::move(result[2]);
          EXPECT_EQ(fut_c.GetValue0(), 30);
          return trpc::MakeReadyFuture<>();
        });
  ASSERT_TRUE(flag);
}

// Summary: WhenAll input iterator with member future unready, then SetValue/SetException one by one.
// Expected: Callback triggered after all futures turned resulted.
TEST(WhenAll, IterateWithUnreadyFuture) {
  bool flag = false;
  trpc::Promise<int> pr_a;
  trpc::Promise<int> pr_b;
  trpc::Promise<int> pr_c;
  std::vector<trpc::Future<int>> vec;
  vec.push_back(pr_a.GetFuture());
  vec.push_back(pr_b.GetFuture());
  vec.push_back(pr_c.GetFuture());
  trpc::WhenAll(vec.begin(), vec.end())
       .Then([&flag](std::vector<trpc::Future<int>>&& result) {
          flag = true;
          trpc::Future<int> fut_a = std::move(result[0]);
          EXPECT_EQ(fut_a.GetValue0(), 10);
          trpc::Future<int> fut_b = std::move(result[1]);
          EXPECT_TRUE(fut_b.IsFailed());
          trpc::Future<int> fut_c = std::move(result[2]);
          EXPECT_EQ(fut_c.GetValue0(), 30);
          return trpc::MakeReadyFuture<>();
        });
  ASSERT_FALSE(flag);
  pr_c.SetValue(30);
  ASSERT_FALSE(flag);
  pr_a.SetValue(10);
  ASSERT_FALSE(flag);
  pr_b.SetException(CommonException("Test"));
  ASSERT_TRUE(flag);
}

// Summary: WhenAny with empty vector.
// Expected: Return ready future.
TEST(WhenAny, IterateWithEmpty) {
  bool flag = false;
  std::vector<trpc::Future<int>> vec;
  trpc::WhenAny(vec.begin(), vec.end())
       .Then([&flag](trpc::Future<std::size_t, std::tuple<int>>&& fut) {
          flag = true;
          return trpc::MakeReadyFuture<>();
        });
  ASSERT_TRUE(flag);
}

// Summary: WhenAny iterate with multiple unready futures, then SetValue to second one.
// Expected: Got index 1 inside callback.
TEST(WhenAny, IterateWithReadyFuture) {
  std::size_t index = 0;
  trpc::Promise<int> pr_a;
  trpc::Promise<int> pr_b;
  std::vector<trpc::Future<int>> vec;
  vec.push_back(pr_a.GetFuture());
  vec.push_back(pr_b.GetFuture());
  trpc::WhenAny(vec.begin(), vec.end())
       .Then([&index](trpc::Future<std::size_t, std::tuple<int>>&& fut) {
          EXPECT_TRUE(fut.IsReady());
          index = std::get<0>(fut.GetValue());
          return trpc::MakeReadyFuture<>();
        });
  pr_b.SetValue(10);
  ASSERT_EQ(index, 1);
}

// Summary: WhenAny iterate with multiple unready futures, then SetException to second one.
// Expected: Got index 1 inside callback.
TEST(WhenAny, IterateWithExceptionFuture) {
  bool flag = false;
  trpc::Promise<int> pr_a;
  trpc::Promise<int> pr_b;
  std::vector<trpc::Future<int>> vec;
  vec.push_back(pr_a.GetFuture());
  vec.push_back(pr_b.GetFuture());
  trpc::WhenAny(vec.begin(), vec.end())
       .Then([&flag](trpc::Future<std::size_t, std::tuple<int>>&& fut) {
          flag = true;
          EXPECT_TRUE(fut.IsFailed());
          return trpc::MakeReadyFuture<>();
        });
  pr_b.SetException(CommonException("Test"));
  ASSERT_TRUE(flag);
}

// Summary: WhenAnyWithoutException input empty vector.
// Expected: Callback triggered with ready future.
TEST(WhenAnyWithoutException, IterateWithEmpty) {
  bool flag = false;
  std::vector<trpc::Future<int>> vec;
  trpc::WhenAnyWithoutException(vec.begin(), vec.end())
       .Then([&flag](trpc::Future<std::size_t, std::tuple<int>>&& fut) {
          flag = true;
          return trpc::MakeReadyFuture<>();
        });
  ASSERT_TRUE(flag);
}

// Summary: WhenAnyWithoutException input two unready futures, then SetValue to second one.
// Expected: Got index 1 inside callback.
TEST(WhenAnyWithoutException, IterateWithReadyFuture) {
  std::size_t index = 0;
  trpc::Promise<int> pr_a;
  trpc::Promise<int> pr_b;
  std::vector<trpc::Future<int>> vec;
  vec.push_back(pr_a.GetFuture());
  vec.push_back(pr_b.GetFuture());
  trpc::WhenAnyWithoutException(vec.begin(), vec.end())
       .Then([&index](trpc::Future<std::size_t, std::tuple<int>>&& fut) {
          EXPECT_TRUE(fut.IsReady());
          index = std::get<0>(fut.GetValue());
          return trpc::MakeReadyFuture<>();
        });
  ASSERT_EQ(index, 0);
  pr_b.SetValue(20);
  ASSERT_EQ(index, 1);
}

// Summary: WhenAnyWithoutException input two unready futures, then SetException on by one.
// Expected: After second SetException, callback triggered with exceptional future.
TEST(WhenAnyWithoutException, IterateWithExceptionFuture) {
  bool flag = false;
  trpc::Promise<int> pr_a;
  trpc::Promise<int> pr_b;
  std::vector<trpc::Future<int>> vec;
  vec.push_back(pr_a.GetFuture());
  vec.push_back(pr_b.GetFuture());
  trpc::WhenAnyWithoutException(vec.begin(), vec.end())
       .Then([&flag](trpc::Future<std::size_t, std::tuple<int>>&& fut) {
          flag = true;
          EXPECT_TRUE(fut.IsFailed());
          return trpc::MakeReadyFuture<>();
        });
  ASSERT_FALSE(flag);
  pr_b.SetException(CommonException("Test"));
  ASSERT_FALSE(flag);
  pr_a.SetException(CommonException("Test"));
  ASSERT_TRUE(flag);
}

// Future transport.
// Summary: Sync request a normal server service by tcp complex inside main thread.
// Expected: True.
TEST_F(TestFuture, SyncByTcpComplexInMainThreadOk) {
  SyncCall(TestFuture::tcp_complex_proxy_, true);
}

// Summary: Sync request a non-exist server service by tcp complex inside main thread.
// Expected: False.
TEST_F(TestFuture, SyncByTcpComplexInMainThreadNotExist) {
  SyncCall(TestFuture::tcp_complex_proxy_not_exist_, false);
}

// Summary: Sync request a delayed server service by tcp complex inside main thread.
// Expected: False.
TEST_F(TestFuture, SyncByTcpComplexInMainThreadTimeout) {
  SyncTimeoutCall(TestFuture::tcp_complex_proxy_);
}

// Summary: Async request a normal server service by tcp complex inside main thread.
// Expected: True.
TEST_F(TestFuture, AsyncByTcpComplexInMainThreadOk) {
  trpc::Future<bool> fut = AsyncCall(TestFuture::tcp_complex_proxy_);
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.GetValue0());
}

// Summary: Async request a non-exist server service by tcp complex inside main thread.
// Expected: False.
TEST_F(TestFuture, AsyncByTcpComplexInMainThreadNotExist) {
  trpc::Future<bool> fut = AsyncCall(TestFuture::tcp_complex_proxy_not_exist_);
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_FALSE(fut.GetValue0());
}

// Summary: Async request a delayed server service by tcp complex inside main thread.
// Expected: Timeout return by exceptional future.
TEST_F(TestFuture, AsyncByTcpComplexInMainThreadTimeout) {
  trpc::Future<bool> fut = AsyncTimeoutCall(TestFuture::tcp_complex_proxy_);
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.GetValue0());
}

// Summary: Send request without recieve response by tcp complex inside main thread.
// Expected: Success.
TEST_F(TestFuture, SendOnlyByTcpComplexInMainThreadOk) {
  SendOnlyCall(TestFuture::tcp_complex_proxy_);
}

// Summary: Sync request a normal server service by udp complex inside main thread.
// Expected: True.
TEST_F(TestFuture, SyncByUdpComplexInMainThreadOk) {
  SyncCall(TestFuture::udp_complex_proxy_, true);
}

// Summary: Sync request a delayed server service by udp complex inside main thread.
// Expected: False.
TEST_F(TestFuture, SyncByUdpComplexInMainThreadTimeout) {
  SyncTimeoutCall(TestFuture::udp_complex_proxy_);
}

// Summary: Async request a normal server service by udp complex inside main thread.
// Expected: True.
TEST_F(TestFuture, AsyncByUdpComplexInMainThreadOk) {
  trpc::Future<bool> fut = AsyncCall(TestFuture::udp_complex_proxy_);
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.GetValue0());
}

// Summary: Async request a delayed server service by udp complex inside main thread.
// Expected: Timeout return by exceptional future.
TEST_F(TestFuture, AsyncByUdpComplexInMainThreadTimeout) {
  trpc::Future<bool> fut = AsyncTimeoutCall(TestFuture::udp_complex_proxy_);
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.GetValue0());
}

// Summary: Send request without recieve response by udp complex inside main thread.
// Expected: Success.
TEST_F(TestFuture, SendOnlyByUdpComplexInMainThreadOk) {
  SendOnlyCall(TestFuture::udp_complex_proxy_);
}

// Summary: Sync request a normal server service by tcp conn pool inside main thread.
// Expected: True.
TEST_F(TestFuture, SyncByTcpConnPoolInMainThreadOk) {
  SyncCall(TestFuture::tcp_conn_pool_proxy_, true);
}

// Summary: Sync request a non-exist server service by tcp conn pool inside main thread.
// Expected: False.
TEST_F(TestFuture, SyncByTcpConnPoolInMainThreadNotExist) {
  SyncCall(TestFuture::tcp_conn_pool_proxy_not_exist_, false);
}

// Summary: Sync request a delayed server service by tcp conn pool inside main thread.
// Expected: False.
TEST_F(TestFuture, SyncByTcpConnPoolInMainThreadTimeout) {
  SyncTimeoutCall(TestFuture::tcp_conn_pool_proxy_);
}

// Summary: Async request a normal server service by tcp conn pool inside main thread.
// Expected: True.
TEST_F(TestFuture, AsyncByTcpConnPoolInMainThreadOk) {
  trpc::Future<bool> fut = AsyncCall(TestFuture::tcp_conn_pool_proxy_);
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.GetValue0());
}

// Summary: Async request a non-exist server service by tcp conn pool inside main thread.
// Expected: False.
TEST_F(TestFuture, AsyncByTcpConnPoolInMainThreadNotExist) {
  trpc::Future<bool> fut = AsyncCall(TestFuture::tcp_conn_pool_proxy_not_exist_);
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_FALSE(fut.GetValue0());
}

// Summary: Async request a delayed server service by tcp conn pool inside main thread.
// Expected: Timeout return by exceptional future.
TEST_F(TestFuture, AsyncByTcpConnPoolInMainThreadTimeout) {
  trpc::Future<bool> fut = AsyncTimeoutCall(TestFuture::tcp_conn_pool_proxy_);
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.GetValue0());
}

// Summary: Send request without recieve response by tcp conn pool inside main thread.
// Expected: Success.
TEST_F(TestFuture, SendOnlyByTcpConnPoolInMainThreadOk) {
  SendOnlyCall(TestFuture::tcp_conn_pool_proxy_);
}

// Summary: Sync request a normal server service by udp conn pool inside main thread.
// Expected: True.
TEST_F(TestFuture, SyncByUdpConnPoolInMainThreadOk) {
  SyncCall(TestFuture::udp_conn_pool_proxy_, true);
}

// Summary: Sync request a delayed server service by udp conn pool inside main thread.
// Expected: False.
TEST_F(TestFuture, SyncByUdpConnPoolInMainThreadTimeout) {
  SyncTimeoutCall(TestFuture::udp_conn_pool_proxy_);
}

// Summary: Async request a normal server service by udp conn pool inside main thread.
// Expected: True.
TEST_F(TestFuture, AsyncByUdpConnPoolInMainThreadOk) {
  trpc::Future<bool> fut = AsyncCall(TestFuture::udp_conn_pool_proxy_);
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.GetValue0());
}

// Summary: Async request a delayed server service by udp conn pool inside main thread.
// Expected: Timeout return by exceptional future.
TEST_F(TestFuture, AsyncByUdpConnPoolInMainThreadTimeout) {
  trpc::Future<bool> fut = AsyncTimeoutCall(TestFuture::udp_conn_pool_proxy_);
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.GetValue0());
}

// Summary: Send request without recieve response by udp conn pool inside main thread.
// Expected: Success.
TEST_F(TestFuture, SendOnlyByUdpConnPoolInMainThreadOk) {
  SendOnlyCall(TestFuture::udp_conn_pool_proxy_);
}

// Summary: Async request a normal server service by tcp complex inside frame thread.
// Expected: True.
TEST_F(TestFuture, AsyncByTcpComplexInFrameThreadOk) {
  trpc::Future<> swt = SwitchIntoFrameThread();
  trpc::Future<bool> fut = swt.Then([](trpc::Future<>&& fut) {
                                 return AsyncCall(TestFuture::tcp_complex_proxy_);
                               });
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.GetValue0());
}

// Summary: Async request a non-exist server service by tcp complex inside frame thread.
// Expected: False.
TEST_F(TestFuture, AsyncByTcpComplexInFrameThreadNotExist) {
  trpc::Future<> swt = SwitchIntoFrameThread();
  trpc::Future<bool> fut = swt.Then([](trpc::Future<>&& fut) {
                                 return AsyncCall(TestFuture::tcp_complex_proxy_not_exist_);
                               });
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_FALSE(fut.GetValue0());
}

// Summary: Async request a delayed server service by tcp complex inside frame thread.
// Expected: Timeout return by exceptional future.
TEST_F(TestFuture, AsyncByTcpComplexInFrameThreadTimeout) {
  trpc::Future<> swt = SwitchIntoFrameThread();
  trpc::Future<bool> fut = swt.Then([](trpc::Future<>&& fut) {
                                 return AsyncTimeoutCall(TestFuture::tcp_complex_proxy_);
                               });
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.GetValue0());
}

// Summary: Async request a normal server service by udp complex inside frame thread.
// Expected: True.
TEST_F(TestFuture, AsyncByUdpComplexInFrameThreadOk) {
  trpc::Future<> swt = SwitchIntoFrameThread();
  trpc::Future<bool> fut = swt.Then([](trpc::Future<>&& fut) {
                                 return AsyncCall(TestFuture::udp_complex_proxy_);
                               });
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.GetValue0());
}

// Summary: Async request a delayed server service by udp complex inside frame thread.
// Expected: Timeout return by exceptional future.
TEST_F(TestFuture, AsyncByUdpComplexInFrameThreadTimeout) {
  trpc::Future<> swt = SwitchIntoFrameThread();
  trpc::Future<bool> fut = swt.Then([](trpc::Future<>&& fut) {
                                 return AsyncTimeoutCall(TestFuture::udp_complex_proxy_);
                               });
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.GetValue0());
}

// Summary: Async request a normal server service by tcp conn pool inside frame thread.
// Expected: True.
TEST_F(TestFuture, AsyncByTcpConnPoolInFrameThreadOk) {
  trpc::Future<> swt = SwitchIntoFrameThread();
  trpc::Future<bool> fut = swt.Then([](trpc::Future<>&& fut) {
                                 return AsyncCall(TestFuture::tcp_conn_pool_proxy_);
                               });
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.GetValue0());
}

// Summary: Async request a non-exist server service by tcp conn pool inside frame thread.
// Expected: False.
TEST_F(TestFuture, AsyncByTcpConnPoolInFrameThreadNotExist) {
  trpc::Future<> swt = SwitchIntoFrameThread();
  trpc::Future<bool> fut = swt.Then([](trpc::Future<>&& fut) {
                                 return AsyncCall(TestFuture::tcp_conn_pool_proxy_not_exist_);
                               });
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_FALSE(fut.GetValue0());
}

// Summary: Async request a delayed server service by tcp conn pool inside frame thread.
// Expected: Timeout return by exceptional future.
TEST_F(TestFuture, AsyncByTcpConnPoolInFrameThreadTimeout) {
  trpc::Future<> swt = SwitchIntoFrameThread();
  trpc::Future<bool> fut = swt.Then([](trpc::Future<>&& fut) {
                                 return AsyncTimeoutCall(TestFuture::tcp_conn_pool_proxy_);
                               });
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.GetValue0());
}

// Summary: Async request a normal server service by udp conn pool inside frame thread.
// Expected: True.
TEST_F(TestFuture, AsyncByUdpConnPoolInFrameThreadOk) {
  trpc::Future<> swt = SwitchIntoFrameThread();
  trpc::Future<bool> fut = swt.Then([](trpc::Future<>&& fut) {
                                 return AsyncCall(TestFuture::udp_conn_pool_proxy_);
                               });
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.GetValue0());
}

// Summary: Async request a delayed server service by udp conn pool inside frame thread.
// Expected: Timeout return by exceptional future.
TEST_F(TestFuture, AsyncByUdpConnPoolInFrameThreadTimeout) {
  trpc::Future<> swt = SwitchIntoFrameThread();
  trpc::Future<bool> fut = swt.Then([](trpc::Future<>&& fut) {
                                 return AsyncTimeoutCall(TestFuture::udp_conn_pool_proxy_);
                               });
  fut = trpc::future::BlockingGet(std::move(fut));
  ASSERT_TRUE(fut.GetValue0());
}

#ifdef TRPC_SEPARATE_RUNTIME_TEST
// Summary: Sync request a normal server service by tcp complex inside handle thread.
// Expected: True.
TEST_F(TestFuture, SyncByTcpComplexInHandleThreadOk) {
  ExecuteTaskInHandleThread([]() { SyncCall(TestFuture::tcp_complex_proxy_, true); });
}

// Summary: Sync request a non-exist server service by tcp complex inside handle thread.
// Expected: False.
TEST_F(TestFuture, SyncByTcpComplexInHandleThreadNotExist) {
  ExecuteTaskInHandleThread([]() { SyncCall(TestFuture::tcp_complex_proxy_not_exist_, false); });
}

// Summary: Sync request a delayed server service by tcp complex inside handle thread.
// Expected: Timeout return by exceptional future.
TEST_F(TestFuture, SyncByTcpComplexInHandleThreadTimeout) {
  ExecuteTaskInHandleThread([]() { SyncTimeoutCall(TestFuture::tcp_complex_proxy_not_exist_); });
}

// Summary: Sync request a normal server service by udp complex inside handle thread.
// Expected: True.
TEST_F(TestFuture, SyncByUdpComplexInHandleThreadOk) {
  ExecuteTaskInHandleThread([]() { SyncCall(TestFuture::udp_complex_proxy_, true); });
}

// Summary: Sync request a delayed server service by udp complex inside handle thread.
// Expected: Timeout return by exceptional future.
TEST_F(TestFuture, SyncByUdpComplexInHandleThreadTimeout) {
  ExecuteTaskInHandleThread([]() { SyncTimeoutCall(TestFuture::udp_complex_proxy_); });
}

// Summary: Sync request a normal server service by tcp conn pool inside handle thread.
// Expected: True.
TEST_F(TestFuture, SyncByTcpConnPoolInHandleThreadOk) {
  ExecuteTaskInHandleThread([]() { SyncCall(TestFuture::tcp_conn_pool_proxy_, true); });
}

// Summary: Sync request a non-exist server service by tcp conn pool inside handle thread.
// Expected: False.
TEST_F(TestFuture, AsyncByTcpConnPoolInHandleThreadNotExist) {
  ExecuteTaskInHandleThread([]() { SyncCall(TestFuture::tcp_conn_pool_proxy_not_exist_, false); });
}

// Summary: Sync request a delayed server service by tcp conn pool inside handle thread.
// Expected: Timeout return by exceptional future.
TEST_F(TestFuture, SyncByTcpConnPoolInHandleThreadTimeout) {
  ExecuteTaskInHandleThread([]() { SyncTimeoutCall(TestFuture::tcp_conn_pool_proxy_); });
}

// Summary: Sync request a normal server service by udp conn pool inside handle thread.
// Expected: True.
TEST_F(TestFuture, SyncByUdpConnPoolInHandleThreadOk) {
  ExecuteTaskInHandleThread([]() { SyncCall(TestFuture::udp_conn_pool_proxy_, true); });
}

// Summary: Sync request a delayed server service by udp conn pool inside handle thread.
// Expected: Timeout return by exceptional future.
TEST_F(TestFuture, SyncByUdpConnPoolInHandleThreadTimeout) {
  ExecuteTaskInHandleThread([]() { SyncTimeoutCall(TestFuture::udp_conn_pool_proxy_); });
}
#endif

}  // namespace trpc::testing

int main(int argc, char** argv) {
  if (!trpc::testing::ExtractServerAndClientArgs(argc, argv, &test_argc, &client_argv, &server_argv)) {
    exit(EXIT_FAILURE);
  }

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
