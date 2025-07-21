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

#include "trpc/overload_control/common/window.h"

#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::overload_control {
namespace testing {

TEST(WindowTest, Window) {
  Window window(std::chrono::microseconds(1000), 10);

  for (int i = 0; i < 9; ++i) {
    ASSERT_FALSE(window.Touch());
  }
  ASSERT_TRUE(window.Touch());

  window.Reset(std::chrono::microseconds(100), 10);
  ASSERT_FALSE(window.Touch());
  std::this_thread::sleep_for(std::chrono::microseconds(200));
  ASSERT_TRUE(window.Touch());

  ASSERT_EQ(window.GetInterval(), std::chrono::microseconds(100));
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
