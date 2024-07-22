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

#include "fixedwindow_limiter_conf.h"
#include "gtest/gtest.h"

namespace trpc::overload_control::testing {
namespace testing
{
  


class FixedTimeWindowControlConfTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // 默认的配置，用于测试
    default_conf.limit = 500;
    default_conf.window_size = 2;
    default_conf.is_report = true;
  }

  trpc::overload_control::FixedTimeWindowControlConf default_conf;
};

TEST_F(FixedTimeWindowControlConfTest, EncodeDecode) {
  // 将默认配置编码为 YAML
  YAML::Node node = YAML::convert<trpc::overload_control::FixedTimeWindowControlConf>::encode(default_conf);

  // 从 YAML 解码回配置
  trpc::overload_control::FixedTimeWindowControlConf decoded_conf;
  YAML::convert<trpc::overload_control::FixedTimeWindowControlConf>::decode(node, decoded_conf);

  // 验证编码和解码结果一致
  ASSERT_EQ(default_conf.limit, decoded_conf.limit);
  ASSERT_EQ(default_conf.window_size, decoded_conf.window_size);
  ASSERT_EQ(default_conf.is_report, decoded_conf.is_report);
}

TEST_F(FixedTimeWindowControlConfTest, LoadFromYAML) {
  // 模拟一个 YAML 配置文件内容
  std::string yaml_content = R"(
plugins:
  overload_control:
    fixedwindow_limter:
      limit: 500
      window_size: 2
      is_report: true
  )";

  // 加载 YAML 内容
  YAML::Node config = YAML::Load(yaml_content);

  // 解码配置
  trpc::overload_control::FixedTimeWindowControlConf conf;
  YAML::convert<trpc::overload_control::FixedTimeWindowControlConf>::decode(config["plugins"]["overload_control"]["fixedwindow_limter"], conf);

  // 验证解码结果
  ASSERT_EQ(conf.limit, 500);
  ASSERT_EQ(conf.window_size, 2);
  ASSERT_EQ(conf.is_report, true);
}

}  // namespace trpc::testing
}  // namespace trpc::overload_control
#endif
