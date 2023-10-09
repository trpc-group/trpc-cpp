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

#include "trpc/transport/client/future/conn_pool/shared_send_queue.h"

#include <memory>

#include "gtest/gtest.h"

#include "trpc/util/time.h"

namespace trpc::internal::testing {

class SharedSendQueueTest : public ::testing::Test {
 public:
  void SetUp() override { shared_send_queue_ = std::make_unique<SharedSendQueue>(capacity_); }
  void TearDown() override { shared_send_queue_.reset(); }

 protected:
  std::unique_ptr<SharedSendQueue> shared_send_queue_;
  size_t capacity_{16};
};

TEST_F(SharedSendQueueTest, PushAndPop) {
  uint32_t timeout = trpc::time::GetMilliSeconds() + 100;
  for (size_t i = 0; i < capacity_; i++) {
    std::unique_ptr<CTransportReqMsg> req_msg = std::make_unique<CTransportReqMsg>();
    ASSERT_EQ(shared_send_queue_->Push(i, req_msg.get(), timeout), true);
    ASSERT_EQ(shared_send_queue_->Get(i), req_msg.get());
    ASSERT_EQ(shared_send_queue_->Pop(i), req_msg.get());
    ASSERT_EQ(shared_send_queue_->Get(i), nullptr);
  }
}

TEST_F(SharedSendQueueTest, DoTimeout) {
  uint64_t timeout = trpc::time::GetMilliSeconds();
  std::vector<std::unique_ptr<CTransportReqMsg>> req_msgs;
  for (size_t i = 0; i < capacity_; i++) {
    std::unique_ptr<CTransportReqMsg> req_msg = std::make_unique<CTransportReqMsg>();
    ASSERT_EQ(shared_send_queue_->Push(i, req_msg.get(), timeout + (i + 1) * 500), true);
    req_msgs.emplace_back(std::move(req_msg));
  }

  int timeout_num = 0;
  Function<void(const SharedSendQueue::DataIterator&)> timeout_handler =
      [this, &timeout_num](const SharedSendQueue::DataIterator& iter) {
        ASSERT_TRUE(shared_send_queue_->GetIndex(iter) < capacity_);
        CTransportReqMsg* msg = shared_send_queue_->GetAndPop(iter);
        ASSERT_TRUE(msg != nullptr);
        timeout_num++;
      };

  for (size_t i = 0; i < capacity_; i++) {
    shared_send_queue_->DoTimeout(timeout + (i + 1) * 500 + 100, timeout_handler);
  }
}

}  // namespace trpc::internal::testing
