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

#include "trpc/overload_control/concurrency_limiter/concurrency_limiter_conf.h"

#include "gtest/gtest.h"

namespace trpc::overload_control {
namespace testing {

TEST(ConcurrencyLimiterControlConf, All) {
  ConcurrencyLimiterControlConf conf;
  ASSERT_EQ(conf.max_concurrency, 60000);
  ASSERT_EQ(conf.is_report, false);

  YAML::convert<ConcurrencyLimiterControlConf> concurr_yaml;

  YAML::Node concurr_node = concurr_yaml.encode(conf);

  ConcurrencyLimiterControlConf decode_conf;

  ASSERT_EQ(concurr_yaml.decode(concurr_node, decode_conf), true);

  decode_conf.Display();
}
}  // namespace testing
}  // namespace trpc::overload_control

#endif
