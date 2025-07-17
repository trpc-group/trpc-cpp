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

#include "trpc/transport/client/future/common/timingwheel_timeout_queue.h"

#include <memory>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/util/time.h"

namespace trpc::internal::testing {

std::unique_ptr<CTransportReqMsg> CreateReqMsg(uint32_t seq_id, uint32_t timeout,
                                               size_t begin_timestamp_us) {
  auto req_msg = std::make_unique<CTransportReqMsg>();
  req_msg->context = MakeRefCounted<ClientContext>();
  req_msg->context->SetRequestId(seq_id);
  req_msg->context->SetTimeout(timeout);
  req_msg->context->SetBeginTimestampUs(begin_timestamp_us);

  return req_msg;
}

class TimingWheelTimeoutQueueTest : public ::testing::Test {
 public:
  void SetUp() override {
    timeout_queue_ = std::make_unique<TimingWheelTimeoutQueue>(10000);
  }
  void TearDown() override {
    timeout_queue_.reset();
  }

 protected:
  std::unique_ptr<TimingWheelTimeoutQueue> timeout_queue_;
};

TEST_F(TimingWheelTimeoutQueueTest, PushAndPop) {
  uint32_t seq_id = 1000;
  uint32_t timeout = 100;
  size_t begin_timestamp_us = trpc::time::GetMicroSeconds();

  std::unique_ptr<CTransportReqMsg> req_msg = CreateReqMsg(seq_id, timeout, begin_timestamp_us);
  size_t send_time = begin_timestamp_us / 1000 + timeout;

  timeout_queue_->Push(seq_id, req_msg.get(), send_time);

  ASSERT_TRUE(timeout_queue_->Pop(seq_id) != nullptr);
  ASSERT_EQ(timeout_queue_->Size(), 0);
}

TEST_F(TimingWheelTimeoutQueueTest, DoTimeout) {
  int count = 2000;
  std::vector<std::unique_ptr<CTransportReqMsg>> req_msgs;
  size_t begin_timestamp_us = trpc::time::GetMicroSeconds();

  for (int i = 1; i <= count; ++i) {
    std::unique_ptr<CTransportReqMsg> req_msg = CreateReqMsg(i, i, begin_timestamp_us);
    size_t send_time = begin_timestamp_us / 1000 + i;

    timeout_queue_->Push(i, req_msg.get(), send_time);

    req_msgs.emplace_back(std::move(req_msg));
  }

  int timeout_num = 0;
  auto func = [&timeout_num, this](const TimingWheelTimeoutQueue::DataIterator& iter) {
    CTransportReqMsg* msg = timeout_queue_->GetAndPop(iter);
    EXPECT_TRUE(msg != nullptr);
    timeout_num++;
  };

  size_t i = 1;
  while (timeout_num < count) {
    timeout_queue_->DoTimeout(begin_timestamp_us / 1000 + i, func);

    ASSERT_EQ(timeout_queue_->Size(), count - i);

    ++i;
  }

  ASSERT_EQ(timeout_num, count);
}

TEST_F(TimingWheelTimeoutQueueTest, DoTimeoutLevel3AndLevel5) {
  int count = 3;
  std::vector<std::unique_ptr<CTransportReqMsg>> req_msgs;
  size_t begin_timestamp_us = trpc::time::GetMicroSeconds();

  for (int i = 1; i <= count; ++i) {
    size_t timeout_interval_ms = 1024 * pow(64, i);
    std::unique_ptr<CTransportReqMsg> req_msg = CreateReqMsg(i, timeout_interval_ms, begin_timestamp_us);
    size_t send_time = begin_timestamp_us / 1000 + timeout_interval_ms;

    timeout_queue_->Push(i, req_msg.get(), send_time);

    req_msgs.emplace_back(std::move(req_msg));
  }

  int timeout_num = 0;
  auto func = [&timeout_num, this](const TimingWheelTimeoutQueue::DataIterator& iter) {
    CTransportReqMsg* msg = timeout_queue_->GetAndPop(iter);
    EXPECT_TRUE(msg != nullptr);
    timeout_num++;
  };

  timeout_queue_->DoTimeout(begin_timestamp_us / 1000 + 1024 * 64 * 64 * 64, func);

  ASSERT_EQ(timeout_num, count);
}

TEST_F(TimingWheelTimeoutQueueTest, ContainsAndGet) {
  constexpr int count = 2000;
  std::vector<std::unique_ptr<CTransportReqMsg>> req_msgs;
  size_t begin_timestamp_us = trpc::time::GetMicroSeconds();

  for (int i = 1; i <= count; ++i) {
    std::unique_ptr<CTransportReqMsg> req_msg = CreateReqMsg(i, i, begin_timestamp_us);
    size_t send_time = begin_timestamp_us / 1000 + i;

    timeout_queue_->Push(i, req_msg.get(), send_time);

    req_msgs.emplace_back(std::move(req_msg));
  }

  for (int i = 1; i <= count; ++i) {
    ASSERT_TRUE(timeout_queue_->Contains(i));

    CTransportReqMsg* msg = timeout_queue_->Get(i);
    ASSERT_TRUE(msg != nullptr);
    ASSERT_TRUE((msg->context->GetRequestId() >= 1) && (msg->context->GetRequestId() <= count));
  }
}

}  // namespace trpc::internal::testing
