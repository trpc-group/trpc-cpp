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

#include "trpc/util/thread/thread_helper.h"

#include <pthread.h>

#include <chrono>
#include <cstddef>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/util/algorithm/random.h"
#include "trpc/util/thread/cpu.h"

namespace trpc::testing {

TEST(Thread, SetCurrentAffinity) {
  auto nprocs = GetNumberOfProcessorsAvailable();
  for (int j = 0; j != 100; ++j) {
    for (std::size_t i = 0; i != nprocs; ++i) {
      SetCurrentThreadAffinity(std::vector<unsigned>{static_cast<unsigned>(i)});
      if (Random(100) < 10) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      ASSERT_EQ(static_cast<int>(i), GetCurrentProcessorId());
    }
  }
}

TEST(Thread, SetCurrentName) {
  SetCurrentThreadName("asdf");
  char buffer[30] = {};
  pthread_getname_np(pthread_self(), buffer, sizeof(buffer));
  ASSERT_EQ("asdf", std::string(buffer));
}

}  // namespace trpc::testing
