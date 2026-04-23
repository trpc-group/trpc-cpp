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

#include "trpc/overload_control/flow_control/recent_queue.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::overload_control {

namespace testing {

class RecentQueueTest : public ::testing::Test {
 protected:
  void SetUp() override { seq_queue_ = std::make_unique<RecentQueue>(3, static_cast<uint64_t>(1e9)); }

  void TearDown() override { seq_queue_ = nullptr; }

 protected:
  RecentQueuePtr seq_queue_;
};

TEST_F(RecentQueueTest, Add) {
  for (int i{0}; i < 3; ++i) {
    ASSERT_EQ(seq_queue_->Add(), true);
    ASSERT_EQ(seq_queue_->Add(), true);
    ASSERT_EQ(seq_queue_->Add(), true);
    ASSERT_EQ(seq_queue_->Add(), false);
    ASSERT_EQ(seq_queue_->Add(), false);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

TEST_F(RecentQueueTest, ActiveCount) {
  for (int i{0}; i < 3; ++i) {
    ASSERT_EQ(seq_queue_->ActiveCount(), 0);
    ASSERT_EQ(seq_queue_->Add(), true);
    ASSERT_EQ(seq_queue_->Add(), true);
    ASSERT_EQ(seq_queue_->ActiveCount(), 2);
    ASSERT_EQ(seq_queue_->Add(), true);
    ASSERT_EQ(seq_queue_->ActiveCount(), 3);
    ASSERT_EQ(seq_queue_->Add(), false);
    ASSERT_EQ(seq_queue_->Add(), false);
    ASSERT_EQ(seq_queue_->ActiveCount(), 3);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

}  // namespace testing

}  // namespace trpc::overload_control

#endif
