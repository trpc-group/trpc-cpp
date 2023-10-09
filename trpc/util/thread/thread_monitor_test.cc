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

#include "trpc/util/thread/thread_monitor.h"

#include <sys/time.h>
#include <unistd.h>

#include <iostream>
#include <queue>
#include <stdexcept>
#include <system_error>
#include <thread>

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(ThreadMonitorTest, ThreadLockBasicFunctionTest) {
  // Test that the lock can only be acquired once, or test that calling the destructor releases the lock.
  {
    ThreadLock monitor;
    ThreadLock::LockT lck(monitor);
    EXPECT_FALSE(lck.TryAcquire());
  }
  // Test that the same thread can acquire the lock multiple times, or test that calling the destructor releases the
  // lock.
  {
    ThreadRecLock monitor;
    ThreadRecLock::LockT lck(monitor);
    EXPECT_TRUE(lck.TryAcquire());
  }
}

TEST(ThreadMonitorTest, ThreadLockMutithreadTest) {
  // Test the behavior of lock contention in a multi-threaded environment: 
  // both locks should not be acquired by other threads simultaneously.
  // Specific behavior: Thread t2 should be continuously blocked until thread t1 releases the lock.
  {
    ThreadLock monitor;
    std::thread t1([&monitor]() {
      ThreadLock::LockT lck(monitor);
      std::this_thread::sleep_for(std::chrono::seconds(3));
    });
    std::this_thread::sleep_for(std::chrono::seconds(1));  // Ensure that t2 is created after t1.
    std::thread t2([&monitor]() {
      bool acquired = false;
      try {
        ThreadLock::LockT lck(monitor);  // Block and wait for t1 to release the lock.
        acquired = true;
      } catch (std::system_error&) {
        acquired = false;
      }
      EXPECT_TRUE(acquired);
    });
    t1.join();
    t2.join();
  }
  {
    ThreadRecLock monitor;
    std::thread t1([&monitor]() {
      ThreadRecLock::LockT lck(monitor);
      std::this_thread::sleep_for(std::chrono::seconds(3));
    });
    std::this_thread::sleep_for(std::chrono::seconds(1));  // Ensure that t2 is created after t1.
    std::thread t2([&monitor]() {
      bool acquired = false;
      try {
        ThreadRecLock::LockT lck(monitor);  // Block and wait for t1 to release the lock.
        acquired = true;
      } catch (std::system_error&) {
        acquired = false;
      }
      EXPECT_TRUE(acquired);
    });
    t1.join();
    t2.join();
  }
}

// Use the edge cases of the producer-consumer scenario to test the edge cases of condition_variable.
// 1. The producer produces an integer and puts it into the queue each time until the queue is full. When the queue 
//    is full, the producer enters a sleeping state unless it is awakened by the consumer's consumption behavior.
// 2. The consumer consumes an integer from the queue each time until the queue is empty. When the queue is empty, 
//    the consumer enters a sleeping state unless it is awakened by the producer's production behavior.
TEST(ThreadMonitorTest, ThreadConditionvariableTest) {
  // 1 producer and 1 consumer.
  ThreadLock monitor;
  std::queue<int> q;
  const unsigned int q_full_size = 5;  // The queue allows a maximum size of 5.
  const int product_per_producer = 10000;
  std::thread producer, consumer;
  producer = std::thread([&] {
    for (int j = 0; j < product_per_producer; ++j) {
      monitor.Lock();
      if (q.size() == q_full_size) {
        monitor.Wait();
      }
      q.push(1);
      monitor.Notify();
      monitor.Unlock();
    }
  });
  // The consumer consumes a total of producer_num * product_per_producer integers.
  int product_nums = product_per_producer;
  consumer = std::thread([&] {
    while (true) {
      monitor.Lock();
      if (product_nums <= 0) {
        monitor.Unlock();
        break;
      }
      if (q.size() == 0) {
        monitor.Wait();
      }
      product_nums--;
      q.pop();
      monitor.Notify();
      monitor.Unlock();
    }
  });
  consumer.join();
  producer.join();
  EXPECT_EQ(product_nums, 0);
}

TEST(ThreadMonitorTest, ThreadCondionvariableTimeoutTest) {
  // Test that the lock is properly released in case of timeout.
  ThreadLock monitor;
  monitor.Lock();
  EXPECT_FALSE(monitor.TimedWait(1000));
  monitor.Unlock();
}
}  // namespace trpc::testing
