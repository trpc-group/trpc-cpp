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

#include "trpc/overload_control/throttler/throttler_priority_impl.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::overload_control {
namespace testing {

static void MakeThrottlerOptions(Throttler::Options& options) {
  options.ratio_for_accepts = 1.3;
  options.requests_padding = 8;
  options.max_throttle_probability = 0.7;
  options.ema_options.factor = 0.8;
  options.ema_options.interval = std::chrono::milliseconds(100);
}

TEST(ThrottlerOverloadControl, MustOnRequest) {
  Throttler::Options options;
  MakeThrottlerOptions(options);

  Throttler throttler(options);

  ThrottlerPriorityImpl throttler_priority(ThrottlerPriorityImpl::Options{
      .throttler = &throttler,
  });

  // Warm-up.
  for (int i = 0; i < 1000; ++i) {
    // 50% throttled and 50% passed
    throttler.OnResponse(false);
    throttler.OnResponse(true);
  }

  int must_succ = 0;
  int must_fail = 0;
  for (int i = 0; i < 500; ++i) {
    if (throttler_priority.MustOnRequest()) {
      ++must_succ;
    } else {
      ++must_fail;
    }

    throttler_priority.OnSuccess(std::chrono::milliseconds(100));
    throttler_priority.OnError();
  }
  ASSERT_NE(must_succ, 0);
  // MustOnRequest used for operations that will not fail.
  ASSERT_EQ(must_fail, 0);
}

TEST(ThrottlerOverloadControlTest, OnRequest) {
  Throttler::Options options;
  MakeThrottlerOptions(options);

  Throttler throttler(options);

  ThrottlerPriorityImpl throttler_priority(ThrottlerPriorityImpl::Options{
      .throttler = &throttler,
  });

  for (int i = 0; i < 1000; ++i) {
    // 50% throttled and 50% passed
    throttler.OnResponse(false);
    throttler.OnResponse(true);
  }

  int succ = 0;
  int fail = 0;

  for (int i = 0; i < 500; ++i) {
    if (throttler_priority.OnRequest()) {
      ++succ;
    } else {
      ++fail;
    }

    throttler_priority.OnSuccess(std::chrono::milliseconds(100));
    throttler_priority.OnError();
  }

  ASSERT_NE(succ, 0);
  ASSERT_NE(fail, 0);
}

TEST(ThrottlerOverloadControlTest, FillToken) {
  Throttler::Options options;
  MakeThrottlerOptions(options);

  Throttler throttler(options);

  ThrottlerPriorityImpl throttler_priority(ThrottlerPriorityImpl::Options{
      .throttler = &throttler,
  });

  for (int i = 0; i < 10; i++) {
    throttler_priority.FillToken();
  }

  ASSERT_EQ(throttler_priority.OnRequest(), true);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
