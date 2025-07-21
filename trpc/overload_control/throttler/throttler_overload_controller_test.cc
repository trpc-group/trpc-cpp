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

#include "trpc/overload_control/throttler/throttler_overload_controller.h"

#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "trpc/codec/testing/protocol_testing.h"
#include "trpc/overload_control/common/request_priority.h"
#include "trpc/util/random.h"

namespace trpc::overload_control {
namespace testing {

static void MakeThrottlerOptions(ThrottlerOverloadController::Options& options) {
  options.is_report = false;
  options.priority_adapter_options.max_priority = std::numeric_limits<uint8_t>::max(),
  options.priority_adapter_options.lower_step = 0.01;
  options.priority_adapter_options.upper_step = 0.01;
  options.priority_adapter_options.fuzzy_ratio = 0.1;
  options.priority_adapter_options.max_update_interval = std::chrono::milliseconds(10);
  options.priority_adapter_options.max_update_size = 50;
  options.priority_adapter_options.histogram_num = 3;
  options.priority_adapter_options.priority = nullptr;

  options.throttler_options.ratio_for_accepts = 1.1;
  options.throttler_options.requests_padding = 8;
  options.throttler_options.max_throttle_probability = 0.7;
  options.throttler_options.ema_options = {};
}

ClientContextPtr MakeClientContext() {
  ClientContextPtr context = MakeRefCounted<ClientContext>();
  context->SetRequest(std::make_shared<trpc::testing::TestProtocol>());
  return context;
}

TEST(ThrottlerOverloadController, OnRequest) {
  ThrottlerOverloadController::Options options;
  MakeThrottlerOptions(options);

  ThrottlerOverloadController overload_control(options);

  int succ = 0;
  int fail = 0;

  for (int i = 0; i < 4096; ++i) {
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    auto context = MakeClientContext();
    DefaultSetClientPriority(context, i % 256);
    if (!overload_control.OnRequest(context)) {
      ++fail;
      continue;
    }
    ++succ;
  }
  ASSERT_GT(succ, 0);
  ASSERT_GE(fail, 0);
}

TEST(ThrottlerOverloadController, OnResponse) {
  ThrottlerOverloadController::Options options;
  MakeThrottlerOptions(options);

  ThrottlerOverloadController overload_control(options);

  for (int i = 0; i < 4096; ++i) {
    auto context = MakeClientContext();

    double d = ::trpc::Random<double>(0.0, 1.0);
    if (d <= 0.2) {
      // 20% fail by overload
      context->SetStatus(Status(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR, 0, ""));
    } else if (d <= 0.4) {
      // 20% fail by other reason
      context->SetStatus(Status(TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR, 0, ""));
    }

    overload_control.OnResponse(context);
  }
  // It keeps running and does not expire.
  ASSERT_FALSE(overload_control.IsExpired());
}

TEST(ThrottlerOverloadController, IsOverloadException) {
  ASSERT_TRUE(ThrottlerOverloadController::IsOverloadException(Status(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR, 0, "")));
  ASSERT_TRUE(ThrottlerOverloadController::IsOverloadException(Status(TrpcRetCode::TRPC_SERVER_LIMITED_ERR, 0, "")));
  ASSERT_TRUE(ThrottlerOverloadController::IsOverloadException(Status(TrpcRetCode::TRPC_CLIENT_LIMITED_ERR, 0, "")));
  ASSERT_FALSE(ThrottlerOverloadController::IsOverloadException(Status()));
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
