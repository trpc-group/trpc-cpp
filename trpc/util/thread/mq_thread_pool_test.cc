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

#include "trpc/util/thread/mq_thread_pool.h"

#include <future>

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(MQThreadPoolTest, All) {
  trpc::ThreadPoolOption thread_pool_option;
  thread_pool_option.thread_num = 4;
  trpc::MQThreadPool thread_pool(std::move(thread_pool_option));

  std::atomic<uint32_t> init_cnt = 0;
  thread_pool.SetThreadInitFunc([&init_cnt] { ++init_cnt; });
  thread_pool.Start();
  ASSERT_FALSE(thread_pool.IsStop());

  while (init_cnt != thread_pool_option.thread_num) {
    usleep(1000);
  }

  std::promise<int64_t> waiters[10000];
  for (auto& waiter : waiters) {
    thread_pool.AddTask([&waiter] {
      int64_t res = 0;
      for (unsigned int i = 1; i <= 1000000; i++) {
        res += i;
      }
      waiter.set_value(res);
    });
  }

  for (auto& waiter : waiters) {
    ASSERT_EQ(waiter.get_future().get(), 500000500000);
  }

  thread_pool.Stop();

  ASSERT_TRUE(thread_pool.IsStop());
  ASSERT_FALSE(thread_pool.AddTask([]() {}));
}

}  // namespace trpc::testing
