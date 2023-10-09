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

#include "trpc/overload_control/high_percentile/high_percentile_overload_controller.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/overload_control/common/request_priority.h"
#include "trpc/util/random.h"
#include "trpc/codec/testing/protocol_testing.h"

namespace trpc::overload_control {
namespace testing {

static ServerContextPtr MakeTestServerContext() {
  auto context = MakeRefCounted<ServerContext>();
  context->SetRequestMsg(std::make_shared<trpc::testing::TestProtocol>());

  return context;
}

TEST(HighPercentileOverloadControllerTest, OnRequest) {
  HighPercentileOverloadController::Options options;
  options.is_report = false;
  options.expected_max_schedule_delay = std::chrono::milliseconds(5);
  options.expected_max_request_latency = std::chrono::milliseconds(5);
  options.min_concurrency_window_size = 200;
  options.min_max_concurrency = 10;
  options.adapter_options.max_priority = 255;
  options.adapter_options.lower_step = 0.01;
  options.adapter_options.upper_step = 0.01;
  options.adapter_options.fuzzy_ratio = 0.1;
  options.adapter_options.max_update_interval = std::chrono::milliseconds(10);
  options.adapter_options.max_update_size = 50;
  options.adapter_options.histogram_num = 3;
  options.adapter_options.priority = nullptr;
  HighPercentileOverloadController overload_control(options);

  auto context = MakeTestServerContext();
  bool ret = overload_control.OnRequest(context);
  ASSERT_TRUE(ret);
}

TEST(HighPercentileOverloadControllerTest, OnResponse) {
  HighPercentileOverloadController::Options options;
  {
    options.is_report = false;
    options.expected_max_schedule_delay = std::chrono::milliseconds(5);
    options.expected_max_request_latency = std::chrono::milliseconds(5);
    options.min_concurrency_window_size = 200;
    options.min_max_concurrency = 10;
    options.adapter_options.max_priority = 255;
    options.adapter_options.lower_step = 0.01;
    options.adapter_options.upper_step = 0.01;
    options.adapter_options.fuzzy_ratio = 0.1;
    options.adapter_options.max_update_interval = std::chrono::milliseconds(10);
    options.adapter_options.max_update_size = 50;
    options.adapter_options.histogram_num = 3;
    options.adapter_options.priority = nullptr;
  }

  HighPercentileOverloadController overload_control(options);

  int succ = 0;
  int fail = 0;

  // Preheating data.
  for (int i = 0; i < 4096; ++i) {
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    auto context = MakeTestServerContext();
    DefaultSetServerPriority(context, i % 256);

    if (!overload_control.OnRequest(context)) {
      continue;
    }
    overload_control.OnResponse(context);
  }
  // Testing error.
  auto context = MakeTestServerContext();
  context->SetStatus(Status(-1, ""));
  overload_control.OnResponse(context);

  // Increase concurrency.
  for (int i = 0; i < 1024; ++i) {
    auto context = MakeTestServerContext();
    DefaultSetServerPriority(context, i % 256);

    if (!overload_control.OnRequest(context)) {
      ++fail;
      continue;
    }
    ++succ;
  }

  ASSERT_GT(fail, 0);
  ASSERT_GT(succ, 0);
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
