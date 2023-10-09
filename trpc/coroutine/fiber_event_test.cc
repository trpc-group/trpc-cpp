//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/waitable_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/coroutine/fiber_event.h"

#include <atomic>
#include <memory>
#include <thread>

#include "gtest/gtest.h"

#include "trpc/coroutine/testing/fiber_runtime.h"

namespace trpc::testing {

TEST(FiberEvent, EventOnWakeup) {
  RunAsFiber([]() {
    for (int i = 0; i != 1000; ++i) {
      auto ev = std::make_unique<FiberEvent>();
      std::thread t([&] { ev->Set(); });
      ev->Wait();
      t.join();
    }
  });
}

}  // namespace trpc::testing
