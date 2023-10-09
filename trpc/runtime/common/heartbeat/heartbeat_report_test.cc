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

#include "trpc/runtime/common/heartbeat/heartbeat_report.h"

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/metrics/metrics.h"
#include "trpc/metrics/metrics_factory.h"
#include "trpc/metrics/trpc_metrics.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"
#include "trpc/runtime/common/runtime_info_report/runtime_info_reporter.h"
#include "trpc/util/time.h"

namespace trpc::testing {

namespace {

double g_queue_size{0.0};

}  // namespace

class TestHearbeatMetrics : public trpc::Metrics {
 public:
  std::string Name() const override { return "test_heartbeat_metrics"; }

  int ModuleReport(const ModuleMetricsInfo& info) override { return 0; }

  int SingleAttrReport(const SingleAttrMetricsInfo& info) override {
    // if (info != nullptr) {
    g_queue_size = info.value;
    // }
    return 0;
  }

  int MultiAttrReport(const MultiAttrMetricsInfo& info) override { return 0; }

  using trpc::Metrics::ModuleReport;
  using trpc::Metrics::MultiAttrReport;
  using trpc::Metrics::SingleAttrReport;
};

class HeartBeatTaskTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    ASSERT_EQ(TrpcConfig::GetInstance()->Init("trpc/runtime/testing/heartbeat_report_test.yaml"), 0);
    PeripheryTaskScheduler::GetInstance()->Init();
    PeripheryTaskScheduler::GetInstance()->Start();
    metrics::Init();
    MetricsFactory::GetInstance()->Register(MakeRefCounted<TestHearbeatMetrics>());
  }

  static void TearDownTestCase() {
    metrics::Destroy();
    PeripheryTaskScheduler::GetInstance()->Stop();
    PeripheryTaskScheduler::GetInstance()->Join();
  }
};

TEST_F(HeartBeatTaskTest, TestServerHeartBeatReportSwitch) {
  auto heart_beat_report = HeartBeatReport::GetInstance();
  heart_beat_report->SetRunning(true);
  {
    ServiceHeartBeatInfo heart_beat_info;
    heart_beat_info.group_name = "group";
    heart_beat_info.service_name = "service";
    heart_beat_info.host = "127.0.0.1";
    heart_beat_info.port = 1271;
    heart_beat_report->RegisterServiceHeartBeatInfo(std::move(heart_beat_info));
  }

  ASSERT_TRUE(heart_beat_report->IsNeedReportServerHeartBeat());

  heart_beat_report->SetServerHeartBeatReportSwitch(false);
  heart_beat_report->Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  heart_beat_report->Stop();
  ASSERT_FALSE(heart_beat_report->IsNeedReportServerHeartBeat());

  heart_beat_report->SetServerHeartBeatReportSwitch(true);
  heart_beat_report->Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  heart_beat_report->Stop();
  ASSERT_TRUE(heart_beat_report->IsNeedReportServerHeartBeat());
}

TEST_F(HeartBeatTaskTest, TestServiceHeartBeatReportSwitch) {
  auto heart_beat_report = HeartBeatReport::GetInstance();
  heart_beat_report->SetRunning(true);

  ASSERT_TRUE(heart_beat_report->IsNeedReportServiceHeartBeat("service"));

  heart_beat_report->SetServiceHeartBeatReportSwitch("service", false);
  ServiceHeartBeatInfo heart_beat_info;
  heart_beat_report->Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  heart_beat_report->Stop();
  ASSERT_FALSE(heart_beat_report->IsNeedReportServiceHeartBeat("service"));

  heart_beat_report->SetServiceHeartBeatReportSwitch("service", true);
  heart_beat_report->Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  heart_beat_report->Stop();
  ASSERT_TRUE(heart_beat_report->IsNeedReportServiceHeartBeat("service"));
}

// Test the case that the related naming service is empty
TEST_F(HeartBeatTaskTest, CheckRegistryNameEmpty) {
  std::atomic_int32_t report_nums = 0;
  auto heart_beat_report = HeartBeatReport::GetInstance();
  heart_beat_report->SetRunning(true);
  heart_beat_report->SetHeartBeatReportFunction(
      [&report_nums](const std::string&, const ServiceHeartBeatInfo&) { ++report_nums; });

  heart_beat_report->SetRegistryName("");
  heart_beat_report->SetRunning(true);
  {
    ServiceHeartBeatInfo heart_beat_info;
    heart_beat_info.group_name = "group";
    heart_beat_info.service_name = "service";
    heart_beat_info.host = "127.0.0.1";
    heart_beat_info.port = 1271;
    heart_beat_report->RegisterServiceHeartBeatInfo(std::move(heart_beat_info));
  }
  ASSERT_EQ(1, heart_beat_report->GetServiceQueueSize());
  heart_beat_report->Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  heart_beat_report->Stop();

  // heartbeat info is not popped from the queue due to empty registry name
  ASSERT_EQ(1, heart_beat_report->GetServiceQueueSize());
  ASSERT_EQ(0, report_nums);
}

// Test the case that the related naming service isn't empty
TEST_F(HeartBeatTaskTest, ServiceRegistryNameNotEmpty) {
  std::atomic_int32_t report_nums = 0;
  auto heart_beat_report = HeartBeatReport::GetInstance();
  heart_beat_report->SetRegistryName("test_trpc_registry");
  heart_beat_report->SetRunning(true);
  heart_beat_report->SetHeartBeatReportFunction(
      [&report_nums](const std::string&, const ServiceHeartBeatInfo&) { ++report_nums; });

  ASSERT_EQ(1, heart_beat_report->GetServiceQueueSize());
  heart_beat_report->SetRunning(true);
  {
    ServiceHeartBeatInfo heart_beat_info;
    heart_beat_info.group_name = "group";
    heart_beat_info.service_name = "service";
    heart_beat_info.host = "127.0.0.1";
    heart_beat_info.port = 1271;
    heart_beat_report->RegisterServiceHeartBeatInfo(std::move(heart_beat_info));
  }
  ASSERT_EQ(2, heart_beat_report->GetServiceQueueSize());
  heart_beat_report->Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  heart_beat_report->Stop();
  ASSERT_EQ(0, heart_beat_report->GetServiceQueueSize());
  // only report one time due to the heartbeat information is the same
  ASSERT_EQ(1, report_nums);
}

TEST_F(HeartBeatTaskTest, CheckThreadHeartBeat) {
  std::atomic_int32_t report_nums = 0;
  auto heart_beat_report = HeartBeatReport::GetInstance();
  ASSERT_EQ(0, heart_beat_report->GetServiceQueueSize());
  heart_beat_report->SetRunning(true);
  heart_beat_report->SetHeartBeatReportFunction(
      [&report_nums](const std::string&, const ServiceHeartBeatInfo&) { ++report_nums; });

  uint64_t now_ms = trpc::time::GetMilliSeconds();
  // simulate heartbeat for two thread model group: group_1 & group_2
  // instance_name_1 contains io role only
  // instance_name_2 contains io and handle roles
  runtime::StartReportRuntimeInfo();
  {
    ThreadHeartBeatInfo heart_beat_info;
    heart_beat_info.group_name = "group_1";
    heart_beat_info.role = kIo;
    heart_beat_info.id = 0;
    heart_beat_info.pid = 1;
    heart_beat_info.last_time_ms = now_ms;
    heart_beat_info.task_queue_size = 1;
    heart_beat_report->ThreadHeartBeat(std::move(heart_beat_info));

    ASSERT_EQ(g_queue_size, heart_beat_info.task_queue_size);
  }

  runtime::StopReportRuntimeInfo();

  {
    ThreadHeartBeatInfo heart_beat_info;
    heart_beat_info.group_name = "group_1";
    heart_beat_info.role = kIo;
    heart_beat_info.id = 1;
    heart_beat_info.pid = 2;
    heart_beat_info.last_time_ms = now_ms;
    heart_beat_info.task_queue_size = 1;
    heart_beat_report->ThreadHeartBeat(std::move(heart_beat_info));
  }
  {
    ThreadHeartBeatInfo heart_beat_info;
    heart_beat_info.group_name = "group_2";
    heart_beat_info.role = kHandle;
    heart_beat_info.id = 0;
    heart_beat_info.pid = 3;
    heart_beat_info.last_time_ms = now_ms;
    heart_beat_info.task_queue_size = 1;
    heart_beat_report->ThreadHeartBeat(std::move(heart_beat_info));
  }
  {
    ThreadHeartBeatInfo heart_beat_info;
    heart_beat_info.group_name = "group_2";
    heart_beat_info.role = kIo;
    heart_beat_info.id = 1;
    heart_beat_info.pid = 4;
    heart_beat_info.last_time_ms = now_ms;
    heart_beat_info.task_queue_size = 1;
    heart_beat_report->ThreadHeartBeat(std::move(heart_beat_info));
  }

  // register two service: server_1_in_group_1 & server_2_in_group_2
  {
    ServiceHeartBeatInfo heart_beat_info;
    heart_beat_info.group_name = "group_1";
    heart_beat_info.service_name = "service_1_in_group_1";
    heart_beat_info.host = "127.0.0.1";
    heart_beat_info.port = 1273;
    heart_beat_report->RegisterServiceHeartBeatInfo(std::move(heart_beat_info));
  }
  {
    ServiceHeartBeatInfo heart_beat_info;
    heart_beat_info.group_name = "group_2";
    heart_beat_info.service_name = "service_2_in_group_2";
    heart_beat_info.host = "127.0.0.1";
    heart_beat_info.port = 1273;
    heart_beat_report->RegisterServiceHeartBeatInfo(std::move(heart_beat_info));
  }

  // Expected: The result of thread heartbeat detection is ok and
  // All services have reported their heartbeats correctly to the name service.
  heart_beat_report->SetThreadTimeOutSec(100000);
  // wait a while for the periodic task to trigger
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  ASSERT_EQ(2, heart_beat_report->GetServiceQueueSize());
  heart_beat_report->Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  heart_beat_report->Stop();
  ASSERT_EQ(0, heart_beat_report->GetServiceQueueSize());
  ASSERT_EQ(0, heart_beat_report->GetIncompetentInstanceSize());

  ASSERT_EQ(2 + 1, report_nums);

  heart_beat_report->SetThreadTimeOutSec(1);
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  heart_beat_report->Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  heart_beat_report->Stop();
  ASSERT_EQ(2, heart_beat_report->GetIncompetentInstanceSize());
  report_nums = 0;
  heart_beat_report->Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  heart_beat_report->Stop();
  ASSERT_EQ(0 + 1, report_nums);

  // All io thread in group_1 are timeout
  // All io thread in group_2 are timeout, but handle thread isn't timeout
  // Expected: All service don't do heartbeat reporting to naming service
  heart_beat_report->SetRunning(true);
  {
    ThreadHeartBeatInfo heart_beat_info;
    heart_beat_info.group_name = "group_2";
    heart_beat_info.role = kHandle;
    heart_beat_info.id = 0;
    heart_beat_info.pid = 3;
    heart_beat_info.last_time_ms = trpc::time::GetMilliSeconds() + 1000000;
    heart_beat_info.task_queue_size = 1;
    heart_beat_report->ThreadHeartBeat(std::move(heart_beat_info));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  heart_beat_report->Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  heart_beat_report->Stop();
  ASSERT_EQ(2, heart_beat_report->GetIncompetentInstanceSize());
  report_nums = 0;
  heart_beat_report->Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  heart_beat_report->Stop();

  ASSERT_EQ(0 + 1, report_nums);

  // No all io thread in group_1 are timeout
  // Expected: service in group_1 do heartbeat reporting to naming service successfully
  heart_beat_report->SetRunning(true);
  {
    ThreadHeartBeatInfo heart_beat_info;
    heart_beat_info.group_name = "group_1";
    heart_beat_info.role = kIo;
    heart_beat_info.id = 0;
    heart_beat_info.pid = 1;
    heart_beat_info.last_time_ms = trpc::time::GetMilliSeconds() + 1000000;
    heart_beat_info.task_queue_size = 1;
    heart_beat_report->ThreadHeartBeat(std::move(heart_beat_info));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  heart_beat_report->Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  heart_beat_report->Stop();
  ASSERT_EQ(1, heart_beat_report->GetIncompetentInstanceSize());
  report_nums = 0;
  heart_beat_report->Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  heart_beat_report->Stop();

  ASSERT_EQ(1 + 1, report_nums);
}

}  // namespace trpc::testing
