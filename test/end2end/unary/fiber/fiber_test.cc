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

#include "gtest/gtest.h"

#include "test/end2end/common/subprocess.h"
#include "test/end2end/common/util.h"
#include "test/end2end/unary/fiber/fiber.trpc.pb.h"
#include "test/end2end/unary/fiber/fiber_server.h"

int test_argc;
char** client_argv;
char** server_argv;

namespace trpc::testing {

class FiberTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    // Run before the first test case
    test_signal_ = std::make_unique<TestSignaller>();
    server_process_ = std::make_unique<SubProcess>([]() {
      FiberServer server(test_signal_.get());
      server.Main(test_argc, server_argv);
      server.Wait();
    });
    // Wait for the server to start
    test_signal_->ClientWaitToContinue();

    InitializeRuntimeEnv(test_argc, client_argv);
    rpc_proxy_ = ::trpc::GetTrpcClient()->GetProxy<::trpc::testing::fibersvr::FiberServiceServiceProxy>("fiber_client");
  }

  static void TearDownTestSuite() {
    // After the last test case
    // Destroy the client environment
    DestroyRuntimeEnv();
    // kill server
    server_process_.reset();
  }

  void SetUp() override {
    // Run before each test case
  }

  void TearDown() override {
    // Run after each test case
  }

 protected:
  static std::unique_ptr<TestSignaller> test_signal_;
  static std::unique_ptr<SubProcess> server_process_;

 public:
  static std::shared_ptr<::trpc::testing::fibersvr::FiberServiceServiceProxy> rpc_proxy_;
};

std::unique_ptr<TestSignaller> FiberTest::test_signal_;
std::unique_ptr<SubProcess> FiberTest::server_process_;

std::shared_ptr<::trpc::testing::fibersvr::FiberServiceServiceProxy> FiberTest::rpc_proxy_;

using TestRequest = trpc::testing::fibersvr::TestRequest;
using TestReply = trpc::testing::fibersvr::TestReply;

using TestFunc = std::function<bool(std::string, TestRequest&, TestReply&, int&)>;

bool DoCreateFiber(const ::trpc::testing::fibersvr::TestRequest& request, ::trpc::testing::fibersvr::TestReply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(FiberTest::rpc_proxy_);
  ::trpc::Status status = FiberTest::rpc_proxy_->TestFiber(ctx, request, &reply);

  return status.OK();
}

bool DoFiberMutex(const ::trpc::testing::fibersvr::TestRequest& request, ::trpc::testing::fibersvr::TestReply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(FiberTest::rpc_proxy_);
  ::trpc::Status status = FiberTest::rpc_proxy_->TestFiberMutex(ctx, request, &reply);

  return status.OK();
}

bool DoFiberConditionVariable(const ::trpc::testing::fibersvr::TestRequest& request,
                              ::trpc::testing::fibersvr::TestReply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(FiberTest::rpc_proxy_);
  ::trpc::Status status = FiberTest::rpc_proxy_->TestFiberConditionVariable(ctx, request, &reply);

  return status.OK();
}

bool DoFiberLatch(const ::trpc::testing::fibersvr::TestRequest& request, ::trpc::testing::fibersvr::TestReply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(FiberTest::rpc_proxy_);
  ::trpc::Status status = FiberTest::rpc_proxy_->TestFiberLatch(ctx, request, &reply);

  return status.OK();
}

bool DoFiberSeqLock(const ::trpc::testing::fibersvr::TestRequest& request,
                    ::trpc::testing::fibersvr::TestReply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(FiberTest::rpc_proxy_);
  ::trpc::Status status = FiberTest::rpc_proxy_->TestFiberSeqLock(ctx, request, &reply);

  return status.OK();
}

bool DoFiberSharedMutex(const ::trpc::testing::fibersvr::TestRequest& request,
                        ::trpc::testing::fibersvr::TestReply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(FiberTest::rpc_proxy_);
  ::trpc::Status status = FiberTest::rpc_proxy_->TestFiberSharedMutex(ctx, request, &reply);

  return status.OK();
}

bool DoFiberTimer(const ::trpc::testing::fibersvr::TestRequest& request, ::trpc::testing::fibersvr::TestReply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(FiberTest::rpc_proxy_);
  ::trpc::Status status = FiberTest::rpc_proxy_->TestFiberTimer(ctx, request, &reply);

  return status.OK();
}

bool DoFiberBlockingBoundedQueue(const ::trpc::testing::fibersvr::TestRequest& request,
                                 ::trpc::testing::fibersvr::TestReply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(FiberTest::rpc_proxy_);
  ::trpc::Status status = FiberTest::rpc_proxy_->TestFiberBlockingBoundedQueue(ctx, request, &reply);

  return status.OK();
}

bool DoFiberBlockingNoncontiguousBuffer(const ::trpc::testing::fibersvr::TestRequest& request,
                                        ::trpc::testing::fibersvr::TestReply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(FiberTest::rpc_proxy_);
  ::trpc::Status status = FiberTest::rpc_proxy_->TestFiberBlockingNoncontiguousBuffer(ctx, request, &reply);

  return status.OK();
}

bool DoFiberEvent(const ::trpc::testing::fibersvr::TestRequest& request, ::trpc::testing::fibersvr::TestReply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(FiberTest::rpc_proxy_);
  ::trpc::Status status = FiberTest::rpc_proxy_->TestFiberEvent(ctx, request, &reply);

  return status.OK();
}

bool DoFiberLocal(const ::trpc::testing::fibersvr::TestRequest& request, ::trpc::testing::fibersvr::TestReply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(FiberTest::rpc_proxy_);
  ::trpc::Status status = FiberTest::rpc_proxy_->TestFiberLocal(ctx, request, &reply);

  return status.OK();
}

bool DoFiberFuture(const ::trpc::testing::fibersvr::TestRequest& request, ::trpc::testing::fibersvr::TestReply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(FiberTest::rpc_proxy_);
  ::trpc::Status status = FiberTest::rpc_proxy_->TestFiberFuture(ctx, request, &reply);

  return status.OK();
}

bool DoFiberAsync(const ::trpc::testing::fibersvr::TestRequest& request, ::trpc::testing::fibersvr::TestReply& reply) {
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(FiberTest::rpc_proxy_);
  ::trpc::Status status = FiberTest::rpc_proxy_->TestFiberAsync(ctx, request, &reply);

  return status.OK();
}

// Create and run a Fiber with default attribute options
// Expectation: Run according to the default scheduling policy
TEST_F(FiberTest, DoCreateFilberDefault) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("default");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoCreateFiber(request, reply));
    ASSERT_TRUE(reply.msg() == "1");
  });
}

// Create and run a Fiber with Dispatch attribute options
// Expectation: Run not according to the default scheduling policy, but on the thread where the Fiber is created
TEST_F(FiberTest, DoCreateFilberDispatch) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("dispatch");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoCreateFiber(request, reply));
    ASSERT_TRUE(reply.msg() == "1");
  });
}

// Create and run a Fiber with the option to be scheduled in another scheduling group
// Expectation: Run in another scheduling group
TEST_F(FiberTest, DoCreateFilberSchedulingGroup) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("scheduling_group");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoCreateFiber(request, reply));
    ASSERT_TRUE(reply.msg() == "0");
  });
}

// Create and run a Fiber with the default StartFiberDetached option
// Expectation: Run according to the default scheduling policy
TEST_F(FiberTest, DoCreateFilberStartFiberDetachedDefault) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("start_fiber_detached_default");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoCreateFiber(request, reply));
    ASSERT_TRUE(reply.msg() != "0");
  });
}

// Create and run a Fiber with the StartFiberDetached and Dispatch attribute options
// Expectation: Run not according to the default scheduling policy, but on the thread where the Fiber is created
TEST_F(FiberTest, DoCreateFilberStartFiberDetachedDispatch) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("start_fiber_detached_dispatch");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoCreateFiber(request, reply));
  });
}

// Lock the FiberMutex normally
// Expectation: try_lock will fai
TEST_F(FiberTest, DoTestFiberMutexNormalCase) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("normal_case");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberMutex(request, reply));
    ASSERT_TRUE(reply.msg() != "0");
  });
}

// Wait on the FiberConditionVariable normally
// Expectation: The awakened coroutine will execute, i.e., run++
TEST_F(FiberTest, DoTestFiberConditionVariableWait) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("wait");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberConditionVariable(request, reply));
    ASSERT_TRUE(reply.msg() == "1");
  });
}

// Wait on the FiberConditionVariable with wait_predicate normally
// Expectation: The awakened coroutine will execute, i.e., run++
TEST_F(FiberTest, DoTestFiberConditionVariableWaitPredicate) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("wait_predicate");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberConditionVariable(request, reply));
    ASSERT_TRUE(reply.msg() == "1");
  });
}

// Wait on the FiberConditionVariable with wait_for normally
// Expectation: The coroutine will execute after the timeout, i.e., run++
TEST_F(FiberTest, DoTestFiberConditionVariableWaitFor) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("wait_for");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberConditionVariable(request, reply));
    ASSERT_TRUE(reply.msg() == "1");
  });
}

// Wait on the FiberConditionVariable with wait_for_predicate normally
// Expectation: The coroutine will execute after the timeout, i.e., run++
TEST_F(FiberTest, DoTestFiberConditionVariableWaitForPredicate) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("wait_for_predicate");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberConditionVariable(request, reply));
    ASSERT_TRUE(reply.msg() == "1");
  });
}

// Wait on the FiberConditionVariable with wait_until normally
// Expectation: The coroutine will execute after the timeout, i.e., run++
TEST_F(FiberTest, DoTestFiberConditionVariableWaitUntil) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("wait_until");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberConditionVariable(request, reply));
    ASSERT_TRUE(reply.msg() == "1");
  });
}

// Wait on the FiberConditionVariable with wait_until_predicate normally
// Expectation: The coroutine will execute after the timeout, i.e., run++
TEST_F(FiberTest, DoTestFiberConditionVariableWaitUntilPredicate) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("wait_until_predicate");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberConditionVariable(request, reply));
    ASSERT_TRUE(reply.msg() == "1");
  });
}

// Wait and CountDown on the FiberLatch normally
// Expectation: The coroutine will execute after the FiberLatch meets the condition, i.e., run++
TEST_F(FiberTest, DoTestFiberLatchWait) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("wait");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberLatch(request, reply));
    ASSERT_TRUE(reply.msg() == "1");
  });
}

// Wait_for on the FiberLatch normally
// Expectation: The FiberLatch will block and wait for a timeout
TEST_F(FiberTest, DoTestFiberLatchWaitFor) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("wait_for");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberLatch(request, reply));
    ASSERT_TRUE(reply.msg() == "0");
  });
}

// Wait_until on the FiberLatch normally
// Expectation: The FiberLatch will block and wait for a timeout
TEST_F(FiberTest, DoTestFiberLatchWaitUntil) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("wait_until");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberLatch(request, reply));
    ASSERT_TRUE(reply.msg() == "0");
  });
}

// Delayed write-priority lock on the FiberSeqLock normally
// Expectation: Return normally
TEST_F(FiberTest, DoTestFiberSeqLock) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("normal");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberSeqLock(request, reply));
  });
}

// Acquire read-write lock normally on the FiberSharedMutex, with write priority if both are present
// Expectation: Return normally
TEST_F(FiberTest, DoTestFiberSharedMutexWriteRead) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("write_read");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberSharedMutex(request, reply));
  });
}

// Perform repeated read operations on the FiberSharedMutex normally
// Expectation: Able to acquire multiple read locks repeatedly
TEST_F(FiberTest, DoTestFiberSharedMutexReadRead) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("read_read");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberSharedMutex(request, reply));
    ASSERT_TRUE(reply.msg() == "1");
  });
}

// Create a Fiber timer using SetFiberTimer
// Expectation: Execute at the scheduled time
TEST_F(FiberTest, DoTestFiberTimerSetFiberTimer) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("set_fiber_timer");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberTimer(request, reply));
    ASSERT_TRUE(reply.msg() >= "10");
  });
}

// Create a repeatable Fiber timer using set_fiber_periodic_timer
// Expectation: Execute at the scheduled time (++called) and repeat execution multiple times
TEST_F(FiberTest, DoTestFiberTimerSetPeriodicFiberTimer) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("set_fiber_periodic_timer");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberTimer(request, reply));
    ASSERT_TRUE(reply.msg() >= "10");
  });
}

// Use FiberTimerKiller to wrap FiberTimer and automatically release FiberTimer resources
// Expectation: Execute at the scheduled time
TEST_F(FiberTest, DoTestFiberTimerFiberTimerKiller) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("fiber_timer_killer");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberTimer(request, reply));
    ASSERT_TRUE(reply.msg() >= "10");
  });
}

// Use FiberBlockingBoundedQueue for Push and Pop operations
// Expectation: Perform Push and Pop operations in different Fibers
TEST_F(FiberTest, DoTestFiberBlockingBoundedQueueNormal) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("normal");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberBlockingBoundedQueue(request, reply));
    ASSERT_TRUE(reply.msg() == "1");
  });
}

// FiberBlockingBoundedQueue performs Push and Pop operations separately within a specified timeout period
// Expectation: Perform Push and Pop operations within the timeout period
TEST_F(FiberTest, DoTestFiberBlockingBoundedQueueExpiresIn) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("expires_in");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberBlockingBoundedQueue(request, reply));
    ASSERT_TRUE(reply.msg() == "1");
  });
}

// FiberBlockingBoundedQueue performs Push and Pop operations separately within a specified timeout period
// Expectation: Execute Push and Pop operations within the timeout period
TEST_F(FiberTest, DoTestFiberBlockingBoundedQueueExpiresAt) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("expires_at");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberBlockingBoundedQueue(request, reply));
    ASSERT_TRUE(reply.msg() == "1");
  });
}

// FiberBlockingBoundedQueue executes TryPush and TryPop operations
// Expectation: Perform TryPush and TryPop operations normally
TEST_F(FiberTest, DoTestFiberBlockingBoundedQueueTry) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("try");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberBlockingBoundedQueue(request, reply));
    ASSERT_TRUE(reply.msg() == "1");
  });
}

// FiberBlockingNoncontiguousBuffer executes Append and Cut operations
// Expectation: Perform Append and Cut operations normally
TEST_F(FiberTest, DoTestFiberBlockingNoncontiguousBufferNormal) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("normal");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberBlockingNoncontiguousBuffer(request, reply));
    ASSERT_TRUE(reply.msg() == "1");
  });
}

// FiberBlockingNoncontiguousBuffer performs Append and Cut operations separately within a specified timeout period
// Expectation: Execute Append and Cut operations within the timeout period
TEST_F(FiberTest, DoTestFiberBlockingNoncontiguousBufferExpiresIn) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("expires_in");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberBlockingNoncontiguousBuffer(request, reply));
    ASSERT_TRUE(reply.msg() == "1");
  });
}

// FiberBlockingNoncontiguousBuffer performs Append and Cut operations separately within a specified timeout period
// Expectation: Perform Append and Cut operations within the timeout period
TEST_F(FiberTest, DoTestFiberBlockingNoncontiguousBufferExpiresAt) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("expires_at");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberBlockingNoncontiguousBuffer(request, reply));
    ASSERT_TRUE(reply.msg() == "1");
  });
}

// FiberBlockingNoncontiguousBuffer executes TryAppend and TryCut operations separately
// Expectation: Perform TryAppend and TryCut operations
TEST_F(FiberTest, DoTestFiberBlockingNoncontiguousBufferTry) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("try");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberBlockingNoncontiguousBuffer(request, reply));
    ASSERT_TRUE(reply.msg() == "1");
  });
}

// FiberBlockingNoncontiguousBuffer executes AppendAlways operation until the buffer is empty
// Expectation: Perform AppendAlways and TryCut operations
TEST_F(FiberTest, DoTestFiberBlockingNoncontiguousBufferAppendAlways) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("append_always");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberBlockingNoncontiguousBuffer(request, reply));
    ASSERT_TRUE(reply.msg() == "helloworld");
  });
}

// Synchronize using FiberEvent in an external thread
// Expectation: Block in the Fiber and wake up in the external thread
TEST_F(FiberTest, DoTestFiberEventNormal) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("normal");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberEvent(request, reply));
    ASSERT_FALSE(reply.msg().empty());
  });
}

// Use FiberLocal to pass data between different Fibers
// Expectation: Synchronize data between different Fibers
TEST_F(FiberTest, DoTestFiberLocalNormal) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("normal");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberLocal(request, reply));
    ASSERT_FALSE(reply.msg().empty());
  });
}

// Synchronize using the Future pattern in a Fiber environment
// Expectation: Synchronize using the Future pattern
TEST_F(FiberTest, DoTestFiberFutureNormal) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("normal");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberFuture(request, reply));
    ASSERT_FALSE(reply.msg().empty());
  });
}

// Use the Future pattern with BlockingTryGet in a Fiber environment to be awakened within a timeout period
// Expectation: Be awakened within the timeout period using the Future pattern
TEST_F(FiberTest, DoTestFiberFutureTryGet) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("try_get");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberFuture(request, reply));
    ASSERT_FALSE(reply.msg().empty());
  });
}

// Use the Future pattern with BlockingTryGet in a Fiber environment to trigger a timeout when not awakened within the
// timeout period Expectation: Trigger a timeout using the Future pattern
TEST_F(FiberTest, DoTestFiberFutureTryGetTimeout) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("try_get_timeout");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberFuture(request, reply));
    ASSERT_TRUE(reply.msg().empty());
  });
}

// Use the combination of Async and Future programming in a Fiber environment
// Expectation: Return normally
TEST_F(FiberTest, DoTestFiberAsyncNormal) {
  RunByEnv([&]() {
    ::trpc::testing::fibersvr::TestRequest request;
    request.set_msg("normal");
    ::trpc::testing::fibersvr::TestReply reply;
    ASSERT_TRUE(DoFiberAsync(request, reply));
    ASSERT_TRUE(reply.msg() >= "1");
  });
}

}  // namespace trpc::testing

int main(int argc, char** argv) {
  if (!::trpc::testing::ExtractServerAndClientArgs(argc, argv, &test_argc, &client_argv, &server_argv)) {
    exit(EXIT_FAILURE);
  }

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
