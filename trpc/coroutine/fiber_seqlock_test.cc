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

#include <iostream>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/fiber_seqlock.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/util/chrono/chrono.h"
#include "trpc/util/latch.h"
#include "trpc/util/likely.h"
#include "trpc/util/random.h"
#include "trpc/util/time.h"

namespace trpc {
namespace testing {

std::vector<unsigned> randomValue;

TEST(FiberSeqLock, genRandom) {
  int i = 1000;
  while (i != 0) {
    randomValue.push_back(Random(1000));
    --i;
  }
}

TEST(FiberSeqLock, seqlock_900_pre_random_rwlock) {
  RunAsFiber([] {
    FiberSeqLock seqLock;
    int cnt1 = 0;
    int cnt2 = 0;
    FiberLatch l(10000);
    for (int i = 0; i != 10000; ++i) {
      StartFiberDetached([&] {
        auto randomLength = randomValue.size();
        for (size_t i = 0; i < randomLength; ++i) {
          auto op = randomValue[i];
          if (op < 900) {  // Read lock
            int cnt1_tmp = 0;
            int cnt2_tmp = 0;
            unsigned seq;
            do {
              seq = seqLock.ReadSeqBegin();
              cnt1_tmp = cnt1;
              cnt2_tmp = cnt2;
            } while (TRPC_UNLIKELY(seqLock.ReadSeqRetry(seq)));
            EXPECT_EQ(cnt1_tmp, cnt2_tmp);
          } else {  // write lock
            int cnt1_tmp = 0;
            int cnt2_tmp = 0;
            seqLock.WriteSeqLock();
            ++cnt1;
            ++cnt2;
            cnt1_tmp = cnt1;
            cnt2_tmp = cnt2;
            seqLock.WriteSeqUnlock();
            EXPECT_EQ(cnt1_tmp, cnt2_tmp);
          }
        }
        l.CountDown();
      });
    }
    l.Wait();
  });
}

}  // namespace testing
}  // namespace trpc
