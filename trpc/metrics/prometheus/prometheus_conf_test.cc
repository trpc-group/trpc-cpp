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

#include "trpc/metrics/prometheus/prometheus_conf.h"

#include "gtest/gtest.h"
#include "yaml-cpp/yaml.h"

#include "trpc/metrics/prometheus/prometheus_conf_parser.h"

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
}

}  // namespace trpc::testing
