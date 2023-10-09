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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/fiber_limiter/fiber_limiter_conf.h"

#include "gtest/gtest.h"

namespace trpc::overload_control {
namespace testing {

TEST(FiberLimiterControlConf, All) {
  FiberLimiterControlConf conf;
  ASSERT_EQ(conf.max_fiber_count, 60000);
  ASSERT_EQ(conf.is_report, false);

  YAML::convert<FiberLimiterControlConf> concurr_yaml;

  YAML::Node concurr_node = concurr_yaml.encode(conf);

  FiberLimiterControlConf decode_conf;

  ASSERT_EQ(concurr_yaml.decode(concurr_node, decode_conf), true);

  decode_conf.Display();
}
}  // namespace testing
}  // namespace trpc::overload_control

#endif
