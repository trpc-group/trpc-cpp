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

#include "trpc/runtime/common/stats/backup_request_stats.h"

#include "trpc/runtime/common/stats/frame_stats_testing.h"

namespace trpc::testing {

TEST_F(FrameStatTest, BackupRequestStatsTest) {
  BackupRequestStats backup_request_stats;
  auto data1 = backup_request_stats.GetData("service1");
  ASSERT_TRUE(data1.retries_success);
  ASSERT_EQ(data1.retries_success->GetAbsPath(), "/trpc/client/service1/backup_request_success");
  ASSERT_EQ(data1.retries_success->GetValue(), 0);
  ASSERT_TRUE(data1.retries);
  ASSERT_EQ(data1.retries->GetAbsPath(), "/trpc/client/service1/backup_request");
  ASSERT_EQ(data1.retries->GetValue(), 0);

  // Get the same tvar when use the same service name
  auto data2 = backup_request_stats.GetData("service1");
  ASSERT_EQ(data2.retries_success.get(), data1.retries_success.get());
  ASSERT_EQ(data2.retries.get(), data1.retries.get());

  // Get different tvar when use different service name
  auto data3 = backup_request_stats.GetData("service2");
  ASSERT_FALSE(data3.retries_success.get() == data1.retries_success.get());
  ASSERT_FALSE(data3.retries.get() == data1.retries.get());
}

TEST_F(FrameStatTest, CheckReportData) {
  std::string metrics_name = "test_server_stats";

  BackupRequestStats backup_request_stats;
  auto data1 = backup_request_stats.GetData("service1");

  // Don't report with empty data
  auto& report_data = GetAttrReportData();
  report_data.clear();
  backup_request_stats.AsyncReportToMetrics(metrics_name);
  ASSERT_TRUE(report_data.empty());

  // Report with data
  data1.retries->Update(3);
  report_data.clear();
  backup_request_stats.AsyncReportToMetrics(metrics_name);
  ASSERT_EQ(report_data.size(), 2);
  ASSERT_EQ(report_data[data1.retries->GetAbsPath()], 3);

  // Report incremental data
  data1.retries->Update(2);
  data1.retries_success->Update(1);
  report_data.clear();
  backup_request_stats.AsyncReportToMetrics(metrics_name);
  ASSERT_EQ(report_data.size(), 2);
  ASSERT_EQ(report_data[data1.retries->GetAbsPath()], 2);
  ASSERT_EQ(report_data[data1.retries_success->GetAbsPath()], 1);
}

}  // namespace trpc::testing
