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

#include "trpc/runtime/common/stats/server_stats.h"

#include "trpc/runtime/common/stats/frame_stats_testing.h"

namespace trpc::testing {

TEST_F(FrameStatTest, ServerStatsTest) {
  ServerStats stats;
  ASSERT_EQ(stats.GetConnCount(), 0);
  ASSERT_EQ(stats.GetReqCount(), 0);
  ASSERT_EQ(stats.GetLastReqCount(), 0);
  ASSERT_EQ(stats.GetTotalReqCount(), 0);
  ASSERT_EQ(stats.GetFailedReqCount(), 0);
  ASSERT_EQ(stats.GetLastFailedReqCount(), 0);
  ASSERT_EQ(stats.GetAvgDelay(), 0.0);
  ASSERT_EQ(stats.GetAvgLastDelay(), 0.0);
  ASSERT_EQ(stats.GetAvgTotalDelay(), 0.0);

  stats.AddConnCount(1);
  ASSERT_EQ(stats.GetConnCount(), 1);
  stats.AddConnCount(1);
  ASSERT_EQ(stats.GetConnCount(), 2);
  stats.AddConnCount(-1);
  ASSERT_EQ(stats.GetConnCount(), 1);

  stats.AddReqCount(1);
  stats.AddReqCount(2);
  stats.AddFailedReqCount(1);
  stats.AddFailedReqCount(1);
  stats.AddReqDelay(1);
  stats.AddReqDelay(2);
  stats.AddReqDelay(3);
  ASSERT_EQ(stats.GetTotalReqCount(), 3);
  ASSERT_EQ(stats.GetReqCount(), 3);
  ASSERT_EQ(stats.GetLastReqCount(), 0);
  ASSERT_EQ(stats.GetTotalFailedReqCount(), 2);
  ASSERT_EQ(stats.GetFailedReqCount(), 2);
  ASSERT_EQ(stats.GetLastFailedReqCount(), 0);
  ASSERT_EQ(stats.GetAvgTotalDelay(), 2.0);
  ASSERT_EQ(stats.GetAvgDelay(), 2.0);
  ASSERT_EQ(stats.GetAvgLastDelay(), 0.0);
  ASSERT_EQ(stats.GetLastMaxDelay(), 0.0);
  ASSERT_EQ(stats.GetMaxDelay(), 3.0);

  stats.Stats();
  ASSERT_EQ(stats.GetTotalReqCount(), 3);
  ASSERT_EQ(stats.GetReqCount(), 0);
  ASSERT_EQ(stats.GetLastReqCount(), 3);
  ASSERT_EQ(stats.GetTotalFailedReqCount(), 2);
  ASSERT_EQ(stats.GetFailedReqCount(), 0);
  ASSERT_EQ(stats.GetLastFailedReqCount(), 2);
  ASSERT_EQ(stats.GetAvgTotalDelay(), 2.0);
  ASSERT_EQ(stats.GetAvgDelay(), 0);
  ASSERT_EQ(stats.GetAvgLastDelay(), 2.0);
  ASSERT_EQ(stats.GetLastMaxDelay(), 3.0);
  ASSERT_EQ(stats.GetMaxDelay(), 0.0);
  ASSERT_EQ(stats.GetConnCount(), 1);

  stats.AddReqCount(3);
  stats.AddFailedReqCount(1);
  stats.AddReqDelay(1);
  stats.AddReqDelay(1);
  stats.AddReqDelay(4);
  ASSERT_EQ(stats.GetTotalReqCount(), 6);
  ASSERT_EQ(stats.GetReqCount(), 3);
  ASSERT_EQ(stats.GetLastReqCount(), 3);
  ASSERT_EQ(stats.GetTotalFailedReqCount(), 3);
  ASSERT_EQ(stats.GetFailedReqCount(), 1);
  ASSERT_EQ(stats.GetLastFailedReqCount(), 2);
  ASSERT_EQ(stats.GetAvgTotalDelay(), 2.0);
  ASSERT_EQ(stats.GetAvgDelay(), 2.0);
  ASSERT_EQ(stats.GetAvgLastDelay(), 2.0);
  ASSERT_EQ(stats.GetLastMaxDelay(), 3.0);
  ASSERT_EQ(stats.GetMaxDelay(), 4.0);
  stats.AddConnCount(-1);
  ASSERT_EQ(stats.GetConnCount(), 0);
}

}  // namespace trpc::testing
