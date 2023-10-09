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

#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/util/queue/lockfree_queue.h"

// Normal single-threaded testing, testing the normality of queue enqueue/dequeue behavior.
TEST(LockFreeQueueTest, SingleThreadFullEmptyTest) {
  trpc::LockFreeQueue<int> q;
  int ret = q.Init(2);
  EXPECT_EQ(ret, trpc::LockFreeQueue<int>::RT_OK);

  EXPECT_EQ(q.Capacity(), 2);
  EXPECT_EQ(q.Size(), 0);
  EXPECT_EQ(q.Enqueue(1), trpc::LockFreeQueue<int>::RT_OK);
  EXPECT_EQ(q.Size(), 1);
  EXPECT_EQ(q.Enqueue(2), trpc::LockFreeQueue<int>::RT_OK);
  EXPECT_EQ(q.Size(), 2);
  EXPECT_EQ(q.Enqueue(3), trpc::LockFreeQueue<int>::RT_FULL);
  EXPECT_EQ(q.Size(), 2);

  int data;
  EXPECT_EQ(q.Dequeue(data), trpc::LockFreeQueue<int>::RT_OK);
  EXPECT_EQ(q.Size(), 1);
  EXPECT_EQ(1, data);
  EXPECT_EQ(q.Dequeue(data), trpc::LockFreeQueue<int>::RT_OK);
  EXPECT_EQ(q.Size(), 0);
  EXPECT_EQ(2, data);
  EXPECT_EQ(q.Dequeue(data), trpc::LockFreeQueue<int>::RT_EMPTY);
}

bool ProcessCommonTest(size_t queue_size, bool producer_finished) {
  trpc::LockFreeQueue<int> q;
  int ret = q.Init(queue_size);
  // The queue size is a power of 2.
  EXPECT_LE(q.Capacity(), 2 * queue_size);
  EXPECT_GE(q.Capacity(), queue_size);
  EXPECT_EQ(q.Size(), 0);
  EXPECT_EQ(ret, trpc::LockFreeQueue<int>::RT_OK);

  const int kProducerNum = 5, kConsumerNum = 3;
  const int kNumProduceAll = kProducerNum * 100;
  std::thread producer_ls[kProducerNum];

  for (int i = 0; i < kProducerNum; ++i) {
    producer_ls[i] = std::thread([&q] {
      for (int k = 1; k <= 100; k++) {
        while (q.Enqueue(k) != trpc::LockFreeQueue<int>::RT_OK) {
        }
      }
    });
  }

  if (producer_finished) {
    for (int i = 0; i < kProducerNum; ++i) {
      producer_ls[i].join();
    }
  }

  std::thread consumer_ls[kConsumerNum];
  std::vector<int> consumer_fetch_ls[kConsumerNum];
  std::atomic<int> fetch_count(0);

  // consumer thread
  for (int i = 0; i < kConsumerNum; ++i) {
    consumer_ls[i] = std::thread([&q, &consumer_fetch_ls, &fetch_count, i, kNumProduceAll] {
      while (fetch_count.load() < kNumProduceAll) {
        int data;
        if (q.Dequeue(data) == trpc::LockFreeQueue<int>::RT_OK) {
          consumer_fetch_ls[i].push_back(data);
          fetch_count++;
        }
      }
    });
  }

  for (int i = 0; i < kConsumerNum; ++i) {
    consumer_ls[i].join();
  }
  if (!producer_finished) {
    for (int i = 0; i < kProducerNum; ++i) {
      producer_ls[i].join();
    }
  }

  // check correctness
  int fetch_sum = 0;
  for (int i = 0; i < kConsumerNum; ++i) {
    std::vector<int>& consumer_fetch = consumer_fetch_ls[i];
    for (unsigned int j = 0; j < consumer_fetch.size(); ++j) {
      fetch_sum += consumer_fetch[j];
    }
  }
  EXPECT_EQ(fetch_sum, 25250);

  return true;
}

// Simple multi-threaded testing, where consumers only consume after producers have finished producing, testing the
// correctness of multiple threads simultaneously adding or removing data
//    - 5 producers, each producer thread puts 100 numbers (values 1-100) into the queue, a total of 500,
//      the sum of the values is ((1+100)100)/25=25250
//    - 3 consumers, each consumer thread retrieves numbers from the queue without limiting the values they retrieve,
//      so the sum of all the values retrieved by the consumers should also be 25250.
TEST(LockFreeQueueTest, MultiThreadUntilFullEmptySimpleTest) { EXPECT_TRUE(ProcessCommonTest(1023, true)); }

// Complex multi-threaded testing, where producers produce simultaneously and consumers consume, testing the
// correctness of multiple threads mixed adding and removing data
//    - 5 producers, each producer thread puts 100 numbers (values 1-100) into the queue, a total of 500,
//      the sum of the values is ((1+100)100)/25=25250
//    - 3 consumers, each consumer thread retrieves numbers from the queue without limiting the values they retrieve,
//      so the sum of all the values retrieved by the consumers should also be 25250.
TEST(LockFreeQueueTest, MultiThreadUntilFullEmptyComplexTest) { EXPECT_TRUE(ProcessCommonTest(1024, false)); }

// 1. Complex multi-threaded testing, where producers produce simultaneously and consumers consume, testing the
//    correctness of multiple threads mixed adding and removing data
//    - 5 producers, each producer thread puts 100 numbers (values 1-100) into the queue, a total of 500,
//      the sum of the values is ((1+100)100)/25=25250
//    - 3 consumers, each consumer thread retrieves numbers from the queue without limiting the values they retrieve,
//      so the sum of all the values retrieved by the consumers should also be 25250
// 2. Limit the queue length to 256, so that the queue full/empty boundary is frequently triggered, testing whether
//    frequent triggering of the boundary can
TEST(LockFreeQueueTest, MultiThreadUntilFullEmptyComplexBoundaryTest) { EXPECT_TRUE(ProcessCommonTest(256, false)); }
