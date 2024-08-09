//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2024 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/smooth_filter/server_request_roll_queue.h"

#include <memory>

#include "trpc/log/trpc_log.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::overload_control {

namespace testing {

class HitQueueTest : public ::testing::Test {
 protected:
  void SetUp() override { hit_queue_ = std::make_shared<RequestRollQueue>(10); }

  void TearDown() override { hit_queue_ = nullptr; }

 protected:
  std::shared_ptr<RequestRollQueue> hit_queue_;
};

TEST_F(HitQueueTest, NextTimeFrame) {
  
  for (int i = 0; i < 35; ++i) {
    hit_queue_->NextTimeFrame();
  }
  ASSERT_EQ(hit_queue_->ActiveSum(), 0);

  for (int i = 0; i < 35; ++i) {
    hit_queue_->AddTimeHit();
  }
  ASSERT_EQ(hit_queue_->ActiveSum(), 35);

  for (int i = 0; i < 35; ++i) {
    hit_queue_->NextTimeFrame();
  }
  ASSERT_EQ(hit_queue_->AddTimeHit(), 1);
  ASSERT_EQ(hit_queue_->AddTimeHit(), 2);
  hit_queue_->NextTimeFrame();
  ASSERT_EQ(hit_queue_->ActiveSum(), 2);
}

}  // namespace testing

}  // namespace trpc::overload_control

#endif
