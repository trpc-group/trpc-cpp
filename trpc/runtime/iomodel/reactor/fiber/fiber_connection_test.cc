//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/io/descriptor_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/iomodel/reactor/fiber/fiber_connection.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/runtime/fiber_runtime.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_reactor.h"
#include "trpc/util/chrono/chrono.h"
#include "trpc/util/latch.h"

namespace trpc::testing {

std::atomic<std::size_t> cleaned{};

class PipeDesc : public FiberConnection {
 public:
  explicit PipeDesc(Reactor* reactor, int fd) : FiberConnection(reactor) { SetFd(fd); }
  EventAction OnReadable() override { return read_rc_; }
  EventAction OnWritable() override { return EventAction::kReady; }
  void OnError(int err) override {}
  void OnCleanup(CleanupReason reason) override { ++cleaned; }

  void SetReadAction(EventAction act) { read_rc_ = act; }

 private:
  EventAction read_rc_;
  int fd_;
};

int CreatePipe() {
  int fds[2];
  pipe2(fds, O_NONBLOCK | O_CLOEXEC);
  write(fds[1], "asdf", 4);
  close(fds[1]);

  return fds[0];
}

TEST(Descriptor, ConcurrentRestartRead) {
  for (auto&& action :
       {FiberConnection::EventAction::kReady, FiberConnection::EventAction::kSuppress}) {
    for (int i = 0; i < 1; ++i) {
      Fiber fibers[2];
      FiberLatch latch(1);
      int fd = CreatePipe();

      Reactor* reactor = fiber::GetReactor(0, fd);

      RefPtr<PipeDesc> desc = MakeRefCounted<PipeDesc>(reactor, fd);
      desc->EnableEvent(EventHandler::EventType::kWriteEvent);
      desc->SetReadAction(action);
      desc->AttachReactor();

      fibers[0] = Fiber([desc, &latch] {
        latch.Wait();
        desc->RestartReadIn(std::chrono::nanoseconds(0));
      });
      fibers[1] = Fiber([desc, &latch] {
        latch.Wait();
        desc->Kill(FiberConnection::CleanupReason::kClosing);
      });
      latch.CountDown();
      for (auto&& e : fibers) {
        e.Join();
      }
    }
  }
  while (cleaned != 1 * 2) {
    FiberSleepFor(std::chrono::milliseconds(1));
  }
}

}  // namespace trpc::testing

int Start(int argc, char** argv, std::function<int(int, char**)> cb) {
  signal(SIGPIPE, SIG_IGN);

  trpc::fiber::StartRuntime();

  int rc = 0;
  {
    trpc::Latch l(1);
    trpc::StartFiberDetached([&] {
      trpc::fiber::StartAllReactor();

      rc = cb(argc, argv);

      trpc::fiber::TerminateAllReactor();

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
