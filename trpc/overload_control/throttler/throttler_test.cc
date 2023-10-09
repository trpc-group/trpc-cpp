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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/throttler/throttler.h"

#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::overload_control {
namespace testing {

TEST(ThrottlerTest, All) {
  Throttler::Options options;
  {
    options.ratio_for_accepts = 1.3;
    options.ema_options = ThrottlerEma::Options{
        .factor = 0.8,
        .interval = std::chrono::milliseconds(100),
        .reset_interval = std::chrono::seconds(1),
    };
  }

  Throttler throttler(options);

  auto start = std::chrono::steady_clock::now();
  /// Warm-up.
  while (std::chrono::steady_clock::now() - start < std::chrono::seconds(1)) {
    throttler.OnResponse(false);
    throttler.OnResponse(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  int failures = 0;
  for (int i = 0; i < 10000; ++i) {
    if (!throttler.OnRequest()) {
      ++failures;
    }
  }

  ASSERT_GT(failures, 3200);
  ASSERT_LE(failures, 3800);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
