//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/async_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/coroutine/async.h"

#include "gtest/gtest.h"

#include "trpc/coroutine/future.h"
#include "trpc/coroutine/testing/fiber_runtime.h"

using namespace std::literals;

namespace trpc::fiber {

TEST(Async, Execute) {
  testing::RunAsFiber([] {
    for (int i = 0; i != 10; ++i) {
      int rc = 0;
      auto tid = std::this_thread::get_id();
      auto ff = Async(Launch::Dispatch, [&] {
        rc = 1;
        ASSERT_EQ(tid, std::this_thread::get_id());
      });
      fiber::BlockingGet(&ff);
      ASSERT_EQ(1, rc);
      Future<int> f = Async([&] {
        return 5;
      });
      FiberYield();
      ASSERT_EQ(5, fiber::BlockingGet(&f).GetValue0());
    }
  });
}

}  // namespace trpc::fiber
