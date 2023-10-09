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

#include "trpc/runtime/iomodel/reactor/fiber/fiber_reactor.h"

#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/runtime/fiber_runtime.h"
#include "trpc/util/latch.h"

namespace trpc {

namespace testing {

TEST(ReactorImplTest, TestReactorImpl) {
  FiberReactor::Options reactor_options;
  reactor_options.id = 0;

  std::unique_ptr<Reactor> reactor = std::make_unique<FiberReactor>(reactor_options);
  reactor->Initialize();
  EXPECT_EQ(reactor->Id(), 0);

  EXPECT_EQ(reactor->Name(), "fiber_impl");

  trpc::FiberLatch l(2);
  trpc::StartFiberDetached([&reactor, &l] {
    reactor->Run();
    l.CountDown();
  });

  int ret = 0;
  Reactor::Task reactor_task = [&ret] { ret = 1; };

  reactor->SubmitTask(std::move(reactor_task));

  trpc::StartFiberDetached([&reactor, &l, &ret] {
    while (!ret) {
      FiberYield();
    }
    reactor->Stop();
    reactor->Join();

    l.CountDown();
  });

  l.Wait();

  EXPECT_EQ(ret, 1);
  reactor->Destroy();
}

}  // namespace testing

}  // namespace trpc

int Start(int argc, char** argv, std::function<int(int, char**)> cb) {
  signal(SIGPIPE, SIG_IGN);

  trpc::fiber::StartRuntime();

  int rc = 0;
  {
    trpc::Latch l(1);
    trpc::StartFiberDetached([&] {
      rc = cb(argc, argv);

      l.count_down();
    });
    l.wait();
  }

  trpc::fiber::TerminateRuntime();

  return rc;
}

int InitAndRunAllTests(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return Start(argc, argv, [](auto, auto) { return ::RUN_ALL_TESTS(); });
}

int main(int argc, char** argv) { return InitAndRunAllTests(argc, argv); }
