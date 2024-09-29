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

#include "trpc/metrics/prometheus/prometheus_conf.h"
// #include "trpc/metrics/prometheus/prometheus_conf_parser.h"

#include "gtest/gtest.h"
#include "yaml-cpp/yaml.h"

namespace trpc::testing {

TEST(PrometheusConfig, Load) {
  trpc::PrometheusConfig prometheus_conf;
  prometheus_conf.histogram_module_cfg = {1, 10, 100};
  prometheus_conf.const_labels = {{"key1", "value1"}, {"key2", "value2"}};

  prometheus_conf.Display();

  auto node = YAML::convert<trpc::PrometheusConfig>::encode(prometheus_conf);

  trpc::PrometheusConfig copy_prometheus_conf;
  ASSERT_TRUE(YAML::convert<trpc::PrometheusConfig>::decode(node, copy_prometheus_conf));

  ASSERT_EQ(copy_prometheus_conf.histogram_module_cfg, prometheus_conf.histogram_module_cfg);
  ASSERT_EQ(copy_prometheus_conf.const_labels, prometheus_conf.const_labels);

  ASSERT_FALSE(copy_prometheus_conf.push_mode.enabled);

}

TEST(PrometheusConfig, LoadWithPushMode) {
  trpc::PrometheusConfig prometheus_conf;
  prometheus_conf.histogram_module_cfg = {1, 10, 100, 1000};
  prometheus_conf.const_labels = {{"key1", "value1"}, {"key2", "value2"}};
  prometheus_conf.push_mode.enabled = true;
  prometheus_conf.push_mode.gateway_url = "http://pushgateway:9091";
  prometheus_conf.push_mode.job_name = "test_job";
  prometheus_conf.push_mode.push_interval_seconds = 30;

  prometheus_conf.Display();

  auto node = YAML::convert<trpc::PrometheusConfig>::encode(prometheus_conf);

  trpc::PrometheusConfig copy_prometheus_conf;
  ASSERT_TRUE(YAML::convert<trpc::PrometheusConfig>::decode(node, copy_prometheus_conf));

  ASSERT_EQ(copy_prometheus_conf.histogram_module_cfg, prometheus_conf.histogram_module_cfg);
  ASSERT_EQ(copy_prometheus_conf.const_labels, prometheus_conf.const_labels);
  ASSERT_TRUE(copy_prometheus_conf.push_mode.enabled);
  ASSERT_EQ(copy_prometheus_conf.push_mode.gateway_url, prometheus_conf.push_mode.gateway_url);
  ASSERT_EQ(copy_prometheus_conf.push_mode.job_name, prometheus_conf.push_mode.job_name);
  ASSERT_EQ(copy_prometheus_conf.push_mode.push_interval_seconds, prometheus_conf.push_mode.push_interval_seconds);
}

TEST(PrometheusConfig, LoadFromYAML) {
  const char* yaml_str = R"(
histogram_module_cfg:
  - 1
  - 10
  - 100
  - 1000
const_labels:
  key1: value1
  key2: value2
push_mode:
  enabled: true
  gateway_url: "http://pushgateway:9091"
  job_name: "test_job"
  push_interval_seconds: 30
)";

  YAML::Node node = YAML::Load(yaml_str);
  trpc::PrometheusConfig prometheus_conf;
  ASSERT_TRUE(YAML::convert<trpc::PrometheusConfig>::decode(node, prometheus_conf));

  ASSERT_EQ(prometheus_conf.histogram_module_cfg, std::vector<double>({1, 10, 100, 1000}));
  ASSERT_EQ(prometheus_conf.const_labels, (std::map<std::string, std::string>{{"key1", "value1"}, {"key2", "value2"}}));
  ASSERT_TRUE(prometheus_conf.push_mode.enabled);
  ASSERT_EQ(prometheus_conf.push_mode.gateway_url, "http://pushgateway:9091");
  ASSERT_EQ(prometheus_conf.push_mode.job_name, "test_job");
  ASSERT_EQ(prometheus_conf.push_mode.push_interval_seconds, 30);
}

}  // namespace trpc::testing
