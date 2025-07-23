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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/high_percentile/max_in_every_n.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::overload_control {
namespace testing {

TEST(MaxInEveryNTest, MaxInEveryN) {
  MaxInEveryN max_in_every_n(3);
  ASSERT_EQ(max_in_every_n.Sample(20.0), std::make_pair(false, 20.0));
  ASSERT_EQ(max_in_every_n.Sample(10.0), std::make_pair(false, 20.0));
  ASSERT_EQ(max_in_every_n.Sample(30.0), std::make_pair(true, 30.0));
  ASSERT_EQ(max_in_every_n.Sample(10.0), std::make_pair(false, 10.0));
}

}  // namespace testing
}  // namespace trpc::overload_control

#endif
