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

#pragma once

#include <string>
#include <unordered_map>

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/metrics/metrics.h"
#include "trpc/metrics/metrics_factory.h"

namespace trpc::testing {

static std::unordered_map<std::string, double> single_attr_report_data;

class TestServerStats : public trpc::Metrics {
 public:
  TestServerStats() = default;

  ~TestServerStats() override = default;

  std::string Name() const override { return "test_server_stats"; }

  int ModuleReport(const ModuleMetricsInfo& info) override { return 0; }

  int SingleAttrReport(const SingleAttrMetricsInfo& info) override {
    single_attr_report_data[info.dimension] = info.value;
    return 0;
  }

  int MultiAttrReport(const MultiAttrMetricsInfo& info) override { return 0; }
};

class FrameStatTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    EXPECT_EQ(TrpcConfig::GetInstance()->Init("trpc/runtime/common/stats/server_stats.yaml"), 0);
    trpc::TrpcPlugin::GetInstance()->RegisterPlugins();
    trpc::MetricsFactory::GetInstance()->Register(trpc::MakeRefCounted<TestServerStats>());
  }

  static void TearDownTestCase() {
    trpc::TrpcPlugin::GetInstance()->UnregisterPlugins();
    trpc::MetricsFactory::GetInstance()->Clear();
  }

  std::unordered_map<std::string, double>& GetAttrReportData() { return single_attr_report_data; }
};

}  // namespace trpc::testing
