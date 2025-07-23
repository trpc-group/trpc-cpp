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

#include "trpc/runtime/common/periphery_task_scheduler.h"

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

#include "trpc/common/config/trpc_config.h"
#include "trpc/util/thread/latch.h"

#include "gtest/gtest.h"

using namespace std::literals;

namespace trpc {
namespace testing {

class PeripheryTaskSchedulerTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    PeripheryTaskScheduler::GetInstance()->Init();
    PeripheryTaskScheduler::GetInstance()->Start();
  }

  static void TearDownTestSuite() {
    PeripheryTaskScheduler::GetInstance()->Stop();
    PeripheryTaskScheduler::GetInstance()->Join();
  }
};

class PeripheryTaskSchedulerMultiCoreTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    const_cast<GlobalConfig&>(TrpcConfig::GetInstance()->GetGlobalConfig()).periphery_task_scheduler_thread_num = 3;
    PeripheryTaskScheduler::GetInstance()->Init();
    PeripheryTaskScheduler::GetInstance()->Start();
  }

  static void TearDownTestSuite() {
    PeripheryTaskScheduler::GetInstance()->Stop();
    PeripheryTaskScheduler::GetInstance()->Join();
  }
};

void TestSubmitTask() {
  bool flag = false;
  std::mutex lock;
  Latch latch(1);
  PeripheryTaskScheduler::GetInstance()->SubmitInnerTask([&flag, &latch] {
    flag = true;
    latch.count_down();
  });
  latch.wait();
  ASSERT_TRUE(flag);
}

void TestRemoveTask() {
  std::uint64_t task_id = PeripheryTaskScheduler::GetInstance()->SubmitInnerTask([]() {});
  ASSERT_TRUE(task_id > 0);
  ASSERT_FALSE(PeripheryTaskScheduler::GetInstance()->RemoveInnerTask(task_id + 1));
  ASSERT_TRUE(PeripheryTaskScheduler::GetInstance()->RemoveInnerTask(task_id));
}

void TestSubmitPeriodicTask() {
  int count = 0;
  Latch latch(1);
  std::uint64_t task_id = 0;
  task_id = PeripheryTaskScheduler::GetInstance()->SubmitInnerPeriodicalTask(
      [&count, &latch, &task_id] {
        count += 1;
        if (count == 2) {
          PeripheryTaskScheduler::GetInstance()->RemoveInnerTask(task_id);
          latch.count_down();
        }
      },
      100);

  latch.wait();
  ASSERT_EQ(count, 2);
}

void TestRemoveTaskAdvance() {
  // remove from outside
  int count = 0;
  Latch latch(1);
  std::uint64_t task_id = PeripheryTaskScheduler::GetInstance()->SubmitInnerPeriodicalTask(
      [&count, &latch]() {
        count += 1;
        if (count == 1) {
          latch.count_down();
        }
      },
      20);
  ASSERT_TRUE(task_id > 0);
  latch.wait();
  ASSERT_TRUE(PeripheryTaskScheduler::GetInstance()->RemoveInnerTask(task_id));
  sleep(1);  // 1s
  ASSERT_EQ(count, 1);

  // remove from inside
  int count2 = 0;
  Latch latch2(1);
  std::uint64_t task_id2 = 0;
  task_id2 = PeripheryTaskScheduler::GetInstance()->SubmitInnerPeriodicalTask(
      [&count2, &latch2, &task_id2]() {
        count2 += 1;
        ASSERT_TRUE(PeripheryTaskScheduler::GetInstance()->RemoveInnerTask(task_id2));
        latch2.count_down();
      },
      20);
  latch2.wait();
  sleep(1);  // 1s
  ASSERT_EQ(count2, 1);
}

void TestSubmitPeriodicTaskWithFunc() {
  int count = 0;
  Latch latch(1);
  std::uint64_t task_id = 0;
  std::uint64_t interval = 0;
  task_id = PeripheryTaskScheduler::GetInstance()->SubmitInnerPeriodicalTask(
      [&count, &latch, &task_id] {
        count += 1;
        if (count == 3) {
          PeripheryTaskScheduler::GetInstance()->RemoveInnerTask(task_id);
          latch.count_down();
        }
      },
      [&interval] {
        interval += 200;
        return interval;
      });
  latch.wait();
  ASSERT_EQ(count, 3);
}

void TestSubmitTaskIntergate() {
  bool flag;
  int count2 = 0, count3 = 0, count4 = 0;
  std::uint64_t task_id2 = 0, task_id3 = 0, task_id4 = 0;
  Latch latch1(1), latch2(1), latch3(1), latch4(1);

  PeripheryTaskScheduler::GetInstance()->SubmitInnerTask([&flag, &latch1] {
    flag = true;
    latch1.count_down();
  });

  task_id2 = PeripheryTaskScheduler::GetInstance()->SubmitInnerPeriodicalTask(
      [&count2, &latch2, &task_id2] {
        count2 += 1;
        if (count2 == 2) {
          PeripheryTaskScheduler::GetInstance()->RemoveInnerTask(task_id2);
          latch2.count_down();
        }
      },
      100);

  uint64_t interval = 0;
  task_id3 = PeripheryTaskScheduler::GetInstance()->SubmitInnerPeriodicalTask(
      [&count3, &latch3, &task_id3] {
        count3 += 1;
        if (count3 == 3) {
          PeripheryTaskScheduler::GetInstance()->RemoveInnerTask(task_id3);
          latch3.count_down();
        }
      },
      [&interval] {
        interval += 200;
        return interval;
      });

  // remove from outside
  task_id4 = PeripheryTaskScheduler::GetInstance()->SubmitInnerPeriodicalTask(
      [&count4, &latch4]() {
        count4 += 1;
        if (count4 == 1) {
          latch4.count_down();
        }
      },
      20);
  ASSERT_TRUE(task_id4 > 0);
  latch4.wait();
  ASSERT_TRUE(PeripheryTaskScheduler::GetInstance()->RemoveInnerTask(task_id4));
  sleep(1);  // 1s
  ASSERT_EQ(count4, 1);

  latch1.wait();
  ASSERT_TRUE(flag);

  latch2.wait();
  ASSERT_EQ(count2, 2);

  latch3.wait();
  ASSERT_EQ(count3, 3);
}

void TestSubmitManyTask() {
  std::mutex mtx;
  Latch latch(1);
  int counter = 0;
  for (unsigned int i = 0; i < 100; ++i) {
    PeripheryTaskScheduler::GetInstance()->SubmitInnerTask([&counter, &mtx, &latch] {
      std::unique_lock _(mtx);
      counter += 1;
      if (counter == 100) {
        latch.count_down();
      }
    });
  }

  bool mask[100] = {false};
  uint64_t task_ids[100];
  for (unsigned int i = 0; i < 100; ++i) {
    task_ids[i] =
        PeripheryTaskScheduler::GetInstance()->SubmitInnerPeriodicalTask([&mask, i]() mutable { mask[i] = true; }, 100);
  }

  sleep(1);  // 1s

  for (unsigned int i = 0; i < 100; ++i) {
    PeripheryTaskScheduler::GetInstance()->RemoveInnerTask(task_ids[i]);
  }

  bool flag = true;
  for (unsigned int i = 0; i < 100; ++i) {
    if (!mask[i]) {
      flag = false;
      break;
    }
  }

  latch.wait();
  ASSERT_TRUE(flag);
}

void TestStopAndJoinTask() {
  unsigned counter = 0;
  Latch latch(1);
  std::uint64_t task_id = PeripheryTaskScheduler::GetInstance()->SubmitInnerPeriodicalTask(
      [&counter, &latch] {
        ++counter;
        if (counter == 3) {
          latch.count_down();
        }
      },
      450);
  latch.wait();
  PeripheryTaskScheduler::GetInstance()->StopInnerTask(task_id);
  PeripheryTaskScheduler::GetInstance()->JoinInnerTask(task_id);
  // task was executed at 0ms, 450ms, 900ms
  ASSERT_EQ(counter, 3);
}

TEST_F(PeripheryTaskSchedulerTest, SubmitTaskTest) { TestSubmitTask(); }

TEST_F(PeripheryTaskSchedulerTest, RemoveTaskTest) { TestRemoveTask(); }

TEST_F(PeripheryTaskSchedulerTest, SubmitPeriodicTaskTest) { TestSubmitPeriodicTask(); }

TEST_F(PeripheryTaskSchedulerTest, RemoveTaskAdvanceTest) { TestRemoveTaskAdvance(); }

TEST_F(PeripheryTaskSchedulerTest, SubmitPeriodicTaskWithDynamicIntervalTest) { TestSubmitPeriodicTaskWithFunc(); }

TEST_F(PeripheryTaskSchedulerTest, SubmitTaskIntergateTest) { TestSubmitTaskIntergate(); }

TEST_F(PeripheryTaskSchedulerTest, SubmitManyTaskTest) { TestSubmitManyTask(); }

TEST_F(PeripheryTaskSchedulerTest, TestStopAndJoinTask) { TestStopAndJoinTask(); }

TEST_F(PeripheryTaskSchedulerMultiCoreTest, SubmitTaskTest) { TestSubmitTask(); }

TEST_F(PeripheryTaskSchedulerMultiCoreTest, RemoveTaskTest) { TestRemoveTask(); }

TEST_F(PeripheryTaskSchedulerMultiCoreTest, SubmitPeriodicTaskTest) { TestSubmitPeriodicTask(); }

TEST_F(PeripheryTaskSchedulerMultiCoreTest, RemoveTaskAdvanceTest) { TestRemoveTaskAdvance(); }

TEST_F(PeripheryTaskSchedulerMultiCoreTest, SubmitPeriodicTaskWithDynamicIntervalTest) {
  TestSubmitPeriodicTaskWithFunc();
}

TEST_F(PeripheryTaskSchedulerMultiCoreTest, SubmitTaskIntergateTest) { TestSubmitTaskIntergate(); }

TEST_F(PeripheryTaskSchedulerMultiCoreTest, SubmitManyTaskTest) { TestSubmitManyTask(); }

TEST_F(PeripheryTaskSchedulerMultiCoreTest, TestStopAndJoinTask) { TestStopAndJoinTask(); }

}  // namespace testing
}  // namespace trpc
