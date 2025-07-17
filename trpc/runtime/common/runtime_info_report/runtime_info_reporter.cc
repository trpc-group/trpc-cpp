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

#include <map>

#include "trpc/admin/base_funcs.h"
#include "trpc/common/config/config_helper.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/metrics/metrics_factory.h"
#include "trpc/metrics/trpc_metrics_report.h"
#include "trpc/runtime/common/heartbeat/heartbeat_report.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"
#include "trpc/runtime/common/stats/frame_stats.h"
#include "trpc/runtime/fiber_runtime.h"
#include "trpc/runtime/runtime.h"
#include "trpc/stream/stream_var.h"
#include "trpc/util/thread/process_info.h"
#include "trpc/util/time.h"

namespace trpc::runtime {

namespace {

// Id of reporting task
static uint64_t report_task_id{0};

// Names of metrics plugins
static std::vector<std::string> need_report_metrics{};

constexpr std::string_view kRuntimeInfo = "trpc_runtime_info";

// Check the feature configuration switch and metrics plugin configuration to determine whether runtime information
// needs to be reported.
inline bool InitNeedReportMetricsName() {
  if (!TrpcConfig::GetInstance()->GetGlobalConfig().enable_runtime_report) {
    return false;
  }

  std::vector<std::string> metric_names;
  if (!TrpcConfig::GetInstance()->GetPluginNodes("metrics", metric_names)) {
    return false;
  }

  need_report_metrics = std::move(metric_names);
  return true;
}

// Get all metris plugins
const std::vector<std::string>& GetNeedReportMetricsName() { return need_report_metrics; }

// Used to report task queue size in separate or merge thread model.
void ReportQueueSize(const ThreadHeartBeatInfo& info) {
  SingleAttrMetricsInfo single_attr_info;
  single_attr_info.dimension = fmt::format("{}_{}_{}_queue", info.group_name, GetThreadRoleString(info.role), info.id);
  single_attr_info.name = kRuntimeInfo;
  single_attr_info.value = static_cast<double>(info.task_queue_size);
  single_attr_info.policy = MetricsPolicy::SET;

  TRPC_FMT_TRACE("{} {}", single_attr_info.dimension, single_attr_info.value);

  auto need_report_metrics = runtime::GetNeedReportMetricsName();
  for (auto&& metric : need_report_metrics) {
    auto metrics_ptr = MetricsFactory::GetInstance()->Get(metric);
    if (metrics_ptr) {
      metrics_ptr->SingleAttrReport(single_attr_info);
    }
  }
}

// Update runtime information, and store in the attrs parameter.
void UpdateRuntimeInfo(std::map<std::string, double>* attrs) {
  // cpu utilization
  attrs->insert({"trpc.CpuUsage", admin::GetCpuUsage()});
  // tcp connection count
  attrs->insert({"trpc.TcpNum", FrameStats::GetInstance()->GetServerStats().GetConnCount()});
  // cpu core count
  attrs->insert({"trpc.CpuCoreNum", GetProcessCpuQuota()});
  // process pid
  attrs->insert({"trpc.ProcPid", getpid()});

  // process file descriptor count
  int fd_count = 0;
  admin::GetProcFdCount(&fd_count);
  attrs->insert({"trpc.MaxFdNum", fd_count});

  // memory usage
  admin::ProcStat proc_stat;
  admin::ReadProcState(&proc_stat);
  attrs->insert(
      {"trpc.AllocMem_MB", static_cast<int64_t>(proc_stat.rss) * admin::GetSystemPageSize() / 1'000'000});

  // total number of threads in process
  attrs->insert({"trpc.ThreadNum", proc_stat.num_threads});

  if (runtime::IsInFiberRuntime()) {
    // number of fibers in task queue
    attrs->insert({"trpc.FiberTaskQueueSize", static_cast<int>(trpc::fiber::GetFiberQueueSize())});
    attrs->insert({"trpc.FiberCount", trpc::GetFiberCount()});
  }
}

// Used to report runtime information.
void ReportRuntimeInfo(const std::string& metric_name, const std::map<std::string, double>& attrs) {
  TrpcSingleAttrMetricsInfo info;
  info.plugin_name = metric_name;
  info.single_attr_info.name = kRuntimeInfo;
  info.single_attr_info.policy = MetricsPolicy::SET;

  for (auto&& attr : attrs) {
    info.single_attr_info.dimension = attr.first;
    info.single_attr_info.value = attr.second;
    trpc::metrics::SingleAttrReport(info);
  }

  // report streaming statistical data
  stream::StreamVarHelper::GetInstance()->ReportStreamVarMetrics(metric_name);
}

void Run() {
  std::map<std::string, double> attrs;
  UpdateRuntimeInfo(&attrs);

  auto need_report_metrics = GetNeedReportMetricsName();
  for (auto&& metric : need_report_metrics) {
    ReportRuntimeInfo(metric, attrs);
  }
}

}  // namespace

void StartReportRuntimeInfo() {
  // already started
  if (report_task_id != 0) {
    return;
  }

  if (!InitNeedReportMetricsName()) {
    return;
  }

  // set the queue size reporting callback function, which will be called by the heartbeat reporting task.
  HeartBeatReport::GetInstance()->SetReportMetricFunction([](const ThreadHeartBeatInfo& info) {
    ReportQueueSize(info);
    return;
  });

  // get report time interval, the minimum limit is 1 second.
  uint32_t report_interval = TrpcConfig::GetInstance()->GetGlobalConfig().report_runtime_info_interval > 1000
                                 ? TrpcConfig::GetInstance()->GetGlobalConfig().report_runtime_info_interval
                                 : 1000;

  // start the periodic reporting task.
  report_task_id = PeripheryTaskScheduler::GetInstance()->SubmitInnerPeriodicalTask([]() { Run(); }, report_interval,
                                                                                    "ReportRuntimeInfo");
}

void StopReportRuntimeInfo() {
  // already stopped
  if (report_task_id == 0) {
    return;
  }

  PeripheryTaskScheduler::GetInstance()->StopInnerTask(report_task_id);
  PeripheryTaskScheduler::GetInstance()->JoinInnerTask(report_task_id);

  report_task_id = 0;
}

}  // namespace trpc::runtime
