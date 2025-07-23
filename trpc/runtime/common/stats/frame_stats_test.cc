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

#include "trpc/runtime/common/stats/frame_stats.h"

#include "trpc/runtime/common/stats/frame_stats_testing.h"

namespace trpc::testing {

TEST_F(FrameStatTest, Interface) {
  FrameStats::GetInstance()->Start();

  FrameStats::GetInstance()->GetServerStats().Stats();

  ASSERT_EQ(0, FrameStats::GetInstance()->GetServerStats().GetConnCount());
  ASSERT_EQ(0, FrameStats::GetInstance()->GetServerStats().GetReqCount());
  ASSERT_EQ(0, FrameStats::GetInstance()->GetServerStats().GetLastReqCount());
  ASSERT_EQ(0, FrameStats::GetInstance()->GetServerStats().GetTotalReqCount());
  ASSERT_EQ(0, FrameStats::GetInstance()->GetServerStats().GetFailedReqCount());
  ASSERT_EQ(0, FrameStats::GetInstance()->GetServerStats().GetLastFailedReqCount());
  ASSERT_EQ(0.0, FrameStats::GetInstance()->GetServerStats().GetAvgDelay());
  ASSERT_EQ(0.0, FrameStats::GetInstance()->GetServerStats().GetAvgLastDelay());
  ASSERT_EQ(0.0, FrameStats::GetInstance()->GetServerStats().GetAvgTotalDelay());

  FrameStats::GetInstance()->GetServerStats().AddConnCount(1);
  ASSERT_EQ(1, FrameStats::GetInstance()->GetServerStats().GetConnCount());

  FrameStats::GetInstance()->GetServerStats().AddReqCount(1);
  ASSERT_EQ(1, FrameStats::GetInstance()->GetServerStats().GetReqCount());
  ASSERT_EQ(0, FrameStats::GetInstance()->GetServerStats().GetLastReqCount());
  ASSERT_EQ(1, FrameStats::GetInstance()->GetServerStats().GetTotalReqCount());

  FrameStats::GetInstance()->GetServerStats().AddFailedReqCount(1);
  ASSERT_EQ(1, FrameStats::GetInstance()->GetServerStats().GetFailedReqCount());
  ASSERT_EQ(0, FrameStats::GetInstance()->GetServerStats().GetLastFailedReqCount());

  FrameStats::GetInstance()->GetServerStats().AddReqDelay(1);
  ASSERT_EQ(1.0, FrameStats::GetInstance()->GetServerStats().GetAvgDelay());
  ASSERT_EQ(0.0, FrameStats::GetInstance()->GetServerStats().GetAvgLastDelay());
  ASSERT_EQ(1.0, FrameStats::GetInstance()->GetServerStats().GetAvgTotalDelay());
  ASSERT_EQ(0, FrameStats::GetInstance()->GetServerStats().GetLastMaxDelay());
  ASSERT_EQ(1, FrameStats::GetInstance()->GetServerStats().GetMaxDelay());

  FrameStats::GetInstance()->GetServerStats().Stats();

  FrameStats::GetInstance()->Stop();
}

}  // namespace trpc::testing
