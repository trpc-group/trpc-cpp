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

#include "trpc/overload_control/flow_control/hit_queue.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::overload_control {

namespace testing {

class HitQueueTest : public ::testing::Test {
 protected:
  void SetUp() override { hit_queue_ = std::make_shared<HitQueue>(10); }

  void TearDown() override { hit_queue_ = nullptr; }

 protected:
  std::shared_ptr<HitQueue> hit_queue_;
};

TEST_F(HitQueueTest, NextFrame) {
  for (int i = 0; i < 35; ++i) {
    hit_queue_->NextFrame();
  }
  ASSERT_EQ(hit_queue_->ActiveSum(), 0);

  for (int i = 0; i < 35; ++i) {
    hit_queue_->AddHit();
  }
  ASSERT_EQ(hit_queue_->ActiveSum(), 35);

  for (int i = 0; i < 35; ++i) {
    hit_queue_->NextFrame();
  }
  ASSERT_EQ(hit_queue_->AddHit(), 1);
  ASSERT_EQ(hit_queue_->AddHit(), 2);
  hit_queue_->NextFrame();
  ASSERT_EQ(hit_queue_->ActiveSum(), 2);
}

}  // namespace testing

}  // namespace trpc::overload_control

#endif
