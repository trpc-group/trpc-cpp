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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/throttler/throttler_ema.h"

#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::overload_control {
namespace testing {

TEST(ThrottlerEma, All) {
  ThrottlerEma ema(ThrottlerEma::Options{
      .factor = 0.1,
      .interval = std::chrono::milliseconds(300),
      .reset_interval = std::chrono::seconds(1),
  });

  ASSERT_DOUBLE_EQ(ema.Sum(std::chrono::steady_clock::now()), 0);

  ema.Add(std::chrono::steady_clock::now(), 100);
  ema.Add(std::chrono::steady_clock::now(), 100);
  ema.Add(std::chrono::steady_clock::now(), 100);

  ASSERT_GT(ema.CurrentTotal(), 100);
  ASSERT_DOUBLE_EQ(ema.Sum(std::chrono::steady_clock::now()), 300);

  std::this_thread::sleep_for(std::chrono::seconds(1));
  ASSERT_DOUBLE_EQ(ema.Sum(std::chrono::steady_clock::now()), 0);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
