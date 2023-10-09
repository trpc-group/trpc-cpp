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

#include "trpc/common/config/retry_conf.h"

#include "gtest/gtest.h"

#include "trpc/common/config/retry_conf_parser.h"

namespace trpc::testing {

TEST(RetryHedgingLimitConfig, Parse) {
  YAML::Node node;
  node["max_tokens"] = 10;
  node["token_ratio"] = 2;

  trpc::RetryHedgingLimitConfig config;
  ASSERT_TRUE(YAML::convert<trpc::RetryHedgingLimitConfig>::decode(node, config));
  node = YAML::convert<trpc::RetryHedgingLimitConfig>::encode(config);
  ASSERT_EQ(10, node["max_tokens"].as<int>());
  ASSERT_EQ(2, node["token_ratio"].as<int>());
}

}  // namespace trpc::testing
