//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/thread/spinlock_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/thread/spinlock.h"

#include <thread>

#include "gtest/gtest.h"

#include "trpc/util/thread/latch.h"

namespace trpc::testing {

std::uint64_t counter{};

TEST(Spinlock, All) {
  constexpr auto T = 10;
  constexpr auto N = 10000;
  std::thread ts[10];
  Latch latch(1);
  Spinlock splk;

  for (auto&& t : ts) {
    t = std::thread([&] {
      latch.wait();
      for (int i = 0; i != N; ++i) {
        std::scoped_lock lk(splk);
        ++counter;
      }
    });
  }
  latch.count_down();
  for (auto&& t : ts) {
    t.join();
  }
  ASSERT_EQ(T * N, counter);
}

}  // namespace trpc::testing
