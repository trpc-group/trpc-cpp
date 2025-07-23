//
//
// Copyright (C) 2020 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/chrono_test.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/chrono/chrono.h"

#include <chrono>
#include <thread>

#include "gtest/gtest.h"

using namespace std::literals;

namespace trpc::testing {

TEST(SystemClock, Compare) {
  auto diff = (ReadSystemClock() - std::chrono::system_clock::now()) / 1ms;
  ASSERT_NEAR(diff, 0, 20);
}

TEST(SteadyClock, Compare) {
  auto diff = (ReadSteadyClock() - std::chrono::steady_clock::now()) / 1ms;
  ASSERT_NEAR(diff, 0, 20);
}

}  // namespace trpc::testing
