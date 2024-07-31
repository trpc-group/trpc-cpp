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

#include "slidingwindow_limiter_conf.h"
#include "gtest/gtest.h"

namespace trpc::overload_control::testing {
namespace testing {

class SlidingWindowControlConfTest : public ::testing::Test {
 protected:
  void SetUp() override {
    default_conf.limit = 500;
    default_conf.window_size = 2;
    default_conf.is_report = true;
  }

  trpc::overload_control::SlidingWindowControlConf default_conf;
};

TEST_F(SlidingWindowControlConfTest, EncodeDecode) {
  YAML::Node node = YAML::convert<trpc::overload_control::SlidingWindowControlConf>::encode(default_conf);

  trpc::overload_control::SlidingWindowControlConf decoded_conf;
  YAML::convert<trpc::overload_control::SlidingWindowControlConf>::decode(node, decoded_conf);

  ASSERT_EQ(default_conf.limit, decoded_conf.limit);
  ASSERT_EQ(default_conf.window_size, decoded_conf.window_size);
  ASSERT_EQ(default_conf.is_report, decoded_conf.is_report);
}

TEST_F(SlidingWindowControlConfTest, LoadFromYAML) {
  // Simulate the contents of a YAML configuration file
  std::string yaml_content = R"(
plugins:
  overload_control:
    slidingwindow_limter:
      limit: 500
      window_size: 2
      is_report: true
  )";

  YAML::Node config = YAML::Load(yaml_content);

  trpc::overload_control::SlidingWindowControlConf conf;
  YAML::convert<trpc::overload_control::SlidingWindowControlConf>::decode(config["plugins"]["overload_control"]["slidingwindow_limter"], conf);

  ASSERT_EQ(conf.limit, 500);
  ASSERT_EQ(conf.window_size, 2);
  ASSERT_EQ(conf.is_report, true);
}

}  // namespace trpc::testing
}  // namespace trpc::overload_control
#endif