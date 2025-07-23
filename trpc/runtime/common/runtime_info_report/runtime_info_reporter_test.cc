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

#include "trpc/runtime/common/runtime_info_report/runtime_info_reporter.h"

#include <condition_variable>
#include <mutex>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/admin/base_funcs.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/metrics/metrics_factory.h"
#include "trpc/metrics/trpc_metrics.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"
#include "trpc/runtime/common/stats/frame_stats.h"
#include "trpc/util/thread/process_info.h"

namespace trpc {

namespace testing {

static std::mutex g_mutex;
static std::condition_variable g_cond;
static bool g_flag{false};
static std::vector<SingleAttrMetricsInfo> g_attrs;
constexpr size_t kAttrsNum = 7;

class TestRuntimeMetrics : public trpc::Metrics {
 public:
  TestRuntimeMetrics() = default;

  ~TestRuntimeMetrics() override = default;

  PluginType Type() const override { return PluginType::kMetrics; }

  std::string Name() const override { return "test_runtime_report"; }

  int ModuleReport(const ModuleMetricsInfo& info) override { return 0; }

  int SingleAttrReport(const SingleAttrMetricsInfo& info) override {
    g_attrs.push_back(info);

    if (g_attrs.size() == kAttrsNum) {
      g_flag = true;
      std::unique_lock<std::mutex> lock(g_mutex);
      g_cond.notify_all();
    }
    return 0;
  }

  int MultiAttrReport(const MultiAttrMetricsInfo& info) override { return 0; }
};

TEST(TestRuntimeMetrics, RuntimeReportTest1) {
  trpc::MetricsFactory::GetInstance()->Register(MakeRefCounted<TestRuntimeMetrics>());

  PeripheryTaskScheduler::GetInstance()->Init();
  PeripheryTaskScheduler::GetInstance()->Start();

  int ret = TrpcConfig::GetInstance()->Init("./trpc/runtime/common/runtime_info_report/test.yaml");
  ASSERT_EQ(0, ret);

  ASSERT_TRUE(trpc::metrics::Init());

  runtime::StartReportRuntimeInfo();

  // Waiting for multiple runtime information to be collected.
  {
    std::unique_lock<std::mutex> lock(g_mutex);
    while (!g_flag) g_cond.wait(lock);
  }

  runtime::StopReportRuntimeInfo();

  ASSERT_EQ(g_attrs[0].dimension, "trpc.AllocMem_MB");
  ASSERT_EQ(g_attrs[1].dimension, "trpc.CpuCoreNum");
  ASSERT_EQ(g_attrs[2].dimension, "trpc.CpuUsage");
  ASSERT_EQ(g_attrs[3].dimension, "trpc.MaxFdNum");
  ASSERT_EQ(g_attrs[4].dimension, "trpc.ProcPid");
  ASSERT_EQ(g_attrs[5].dimension, "trpc.TcpNum");
  ASSERT_EQ(g_attrs[6].dimension, "trpc.ThreadNum");

  trpc::metrics::Destroy();
  ASSERT_EQ(g_flag, true);

  PeripheryTaskScheduler::GetInstance()->Stop();
  PeripheryTaskScheduler::GetInstance()->Join();

  trpc::MetricsFactory::GetInstance()->Clear();
}

}  // namespace testing

}  // namespace trpc
