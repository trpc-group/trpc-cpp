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

#pragma once

#include "trpc/metrics/metrics.h"

namespace trpc::testing {

constexpr char kTestMetricsName[] = "test_metrics";

class TestMetrics : public trpc::Metrics {
 public:
  std::string Name() const override { return kTestMetricsName; }

  int Init() noexcept override { return 0; }

  int ModuleReport(const ModuleMetricsInfo& info) override { return 0; }
  int ModuleReport(ModuleMetricsInfo&& info) override { return 0; }

  int SingleAttrReport(const SingleAttrMetricsInfo& info) override { return 0; }
  int SingleAttrReport(SingleAttrMetricsInfo&& info) override { return 0; }

  int MultiAttrReport(const MultiAttrMetricsInfo& info) override { return 0; }
  int MultiAttrReport(MultiAttrMetricsInfo&& info) override { return 0; }
};

}  // namespace trpc::testing
