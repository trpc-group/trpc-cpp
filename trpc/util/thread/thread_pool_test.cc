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

#include "trpc/util/thread/thread_pool.h"

#include <utility>

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(ThreadPoolTest, TestSQThreadPool) {
  trpc::ThreadPoolOption thread_pool_option;
  thread_pool_option.thread_num = 2;
  thread_pool_option.bind_core = true;

  trpc::ThreadPool tp(std::move(thread_pool_option));

  ASSERT_TRUE(tp.GetThreadPoolOption().thread_num == 2);

  ASSERT_TRUE(tp.Start());

  std::atomic<size_t> task_execute_num = 0;
  for (size_t i = 0; i < 10; ++i) {
    bool result = tp.AddTask([&task_execute_num] { ++task_execute_num; });

    ASSERT_TRUE(result);
  }

  tp.Stop();
  tp.Join();

  ASSERT_TRUE(tp.IsStop());
  ASSERT_TRUE(task_execute_num == 10);

  bool result = tp.AddTask([] {});

  ASSERT_FALSE(result);
}

TEST(ThreadPoolTest, TestMQThreadPool) {
  trpc::ThreadPoolOption thread_pool_option;
  thread_pool_option.thread_num = 2;
  thread_pool_option.bind_core = true;

  trpc::ThreadPoolImpl<trpc::MQThreadPool> tp(std::move(thread_pool_option));

  ASSERT_TRUE(tp.GetThreadPoolOption().thread_num == 2);

  ASSERT_TRUE(tp.Start());

  std::atomic<size_t> task_execute_num = 0;
  for (size_t i = 0; i < 10; ++i) {
    bool result = tp.AddTask([&task_execute_num] { ++task_execute_num; });

    ASSERT_TRUE(result);
  }

  tp.Stop();
  tp.Join();

  ASSERT_TRUE(tp.IsStop());
  ASSERT_TRUE(task_execute_num == 10);

  bool result = tp.AddTask([] {});

  ASSERT_FALSE(result);
}

}  // namespace trpc::testing
