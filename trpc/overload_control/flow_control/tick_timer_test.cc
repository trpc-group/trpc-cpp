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

#include "trpc/overload_control/flow_control/tick_timer.h"

#include <chrono>
#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::overload_control {
namespace testing {

class TickTimerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    tick_timer_ = std::make_shared<TickTimer>(std::chrono::microseconds(1000), [this] { OnNextFrame(); });
  }

  void TearDown() override { tick_timer_ = nullptr; }

  void OnNextFrame() { is_run_ = true; }

 protected:
  std::shared_ptr<TickTimer> tick_timer_;
  bool is_run_{false};
};

TEST_F(TickTimerTest, Loop) {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  tick_timer_->Deactivate();
  ASSERT_TRUE(is_run_);
}

}  // namespace testing

}  // namespace trpc::overload_control

#endif
