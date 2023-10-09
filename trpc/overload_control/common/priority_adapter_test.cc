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

#include "trpc/overload_control/common/priority_adapter.h"

#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "trpc/util/random.h"

namespace trpc::overload_control {
namespace testing {

// Mock priority class.
class MockPriority : public Priority {
 public:
  MOCK_METHOD0(MustOnRequest, bool());
  MOCK_METHOD0(OnRequest, bool());
  MOCK_METHOD1(OnSuccess, void(std::chrono::steady_clock::duration));
  MOCK_METHOD0(OnError, void(void));
};

// Test request processing.
TEST(PriorityAdapter, OnRequest) {
  MockPriority mock_priority;
  PriorityAdapter adapter(PriorityAdapter::Options{
      .report_name = "Greeter/SayHello",
      .is_report = false,
      .max_priority = 255,
      .lower_step = 0.02,
      .upper_step = 0.01,
      .fuzzy_ratio = 0.1,
      .max_update_interval = std::chrono::milliseconds(1),
      .max_update_size = 100,
      .histogram_num = 3,
      .priority = &mock_priority,
  });

  EXPECT_CALL(mock_priority, OnRequest()).WillRepeatedly(::testing::Return(true));
  auto ret = adapter.OnRequest(200);
  ASSERT_EQ(ret, PriorityAdapter::Result::kOK);

  // Wait for 3ms to ensure that the window has expired.
  std::this_thread::sleep_for(std::chrono::microseconds(3));

  EXPECT_CALL(mock_priority, OnRequest()).WillRepeatedly(::testing::Return(false));
  ret = adapter.OnRequest(200);
  ASSERT_NE(ret, PriorityAdapter::Result::kOK);
}

// Logic for handling successful and failed test requests.
TEST(PriorityAdapter, OnSuccessAndIgnore) {
  MockPriority mock_priority;
  PriorityAdapter adapter(PriorityAdapter::Options{
      .report_name = "Greeter/SayHello",
      .is_report = false,
      .max_priority = 50,
      .lower_step = 0.02,
      .upper_step = 0.01,
      .fuzzy_ratio = 0.1,
      .max_update_interval = std::chrono::milliseconds(1),
      .max_update_size = 100,
      .histogram_num = 3,
      .priority = &mock_priority,
  });

  EXPECT_CALL(mock_priority, OnSuccess(::testing::_)).WillRepeatedly(::testing::Return());
  // Take a sample to ensure that the sample size is sufficient.
  EXPECT_CALL(mock_priority, OnRequest()).WillRepeatedly(::testing::Return(false));
  for (int i = 0; i < 300; i++) {
    adapter.OnRequest(0);
  }
  adapter.OnSuccess(std::chrono::milliseconds(100));
  // Wait for 3ms to ensure that the window has expired.
  std::this_thread::sleep_for(std::chrono::microseconds(3));
  EXPECT_CALL(mock_priority, OnError()).WillRepeatedly(::testing::Return());
  adapter.OnError();
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
