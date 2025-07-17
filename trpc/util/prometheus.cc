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

#ifdef TRPC_BUILD_INCLUDE_PROMETHEUS
#include "trpc/util/prometheus.h"

#include "trpc/admin/base_funcs.h"
#include "trpc/util/log/logging.h"

namespace trpc::prometheus {

std::shared_ptr<::prometheus::Registry> collector = std::make_shared<::prometheus::Registry>();

std::shared_ptr<::prometheus::Registry> GetRegistry() { return collector; }

::prometheus::Family<::prometheus::Counter>* GetCounterFamily(const char* name, const char* help,
                                                              const std::map<std::string, std::string>& labels) {
  return &::prometheus::BuildCounter().Name(name).Help(help).Labels(labels).Register(*collector);
}

::prometheus::Family<::prometheus::Gauge>* GetGaugeFamily(const char* name, const char* help,
                                                          const std::map<std::string, std::string>& labels) {
  return &::prometheus::BuildGauge().Name(name).Help(help).Labels(labels).Register(*collector);
}

::prometheus::Family<::prometheus::Histogram>* GetHistogramFamily(const char* name, const char* help,
                                                                  const std::map<std::string, std::string>& labels) {
  return &::prometheus::BuildHistogram().Name(name).Help(help).Labels(labels).Register(*collector);
}

::prometheus::Family<::prometheus::Summary>* GetSummaryFamily(const char* name, const char* help,
                                                              const std::map<std::string, std::string>& labels) {
  return &::prometheus::BuildSummary().Name(name).Help(help).Labels(labels).Register(*collector);
}

namespace {

// Process CPU usage time monitoring, in seconds.
::prometheus::Counter* process_cpu_seconds = nullptr;
std::mutex process_cpu_seconds_lock;

// Resident memory used by a process, in bytes.
::prometheus::Gauge* president_memory_bytes = nullptr;

// Virtual memory used by a process, in bytes.
::prometheus::Gauge* virtual_memory_bytes = nullptr;

void UpdateProcessMetric() {
  admin::ProcStat proc_stat;
  int stat_ret = admin::ReadProcState(&proc_stat);
  if (stat_ret != 0) {
    TRPC_LOG_ERROR("get process stat failed, update failed");
    return;
  }

  {
    std::lock_guard<std::mutex> lock(process_cpu_seconds_lock);
    double old_value = process_cpu_seconds->Value();
    // The time unit for utime and stime in proc_stat is the Linux timer interval, and the clock frequency of Linux
    // is obtained from the system.
    static int sys_clk_tck = admin::GetSystemClockTick();
    static int clk_tck = sys_clk_tck > 0 ? sys_clk_tck : 100;
    double cur_value = static_cast<double>(proc_stat.utime + proc_stat.stime) / clk_tck;
    process_cpu_seconds->Increment(cur_value - old_value);
  }

  // The unit for proc_stat.rss is pages, and it needs to be converted to bytes.
  president_memory_bytes->Set(static_cast<int64_t>(proc_stat.rss) * admin::GetSystemPageSize());
  virtual_memory_bytes->Set(proc_stat.vsize);
}

void InitProcessMetrics() {
  ::prometheus::Family<::prometheus::Counter>* process_cpu_seconds_family =
      GetCounterFamily("process_cpu_seconds_total", "Total user and system CPU time spent in seconds.", {});
  process_cpu_seconds = &process_cpu_seconds_family->Add({});

  ::prometheus::Family<::prometheus::Gauge>* president_memory_bytes_family =
      GetGaugeFamily("process_resident_memory_bytes", "Resident memory size in bytes.", {});
  president_memory_bytes = &president_memory_bytes_family->Add({});

  ::prometheus::Family<::prometheus::Gauge>* virtual_memory_bytes_family =
      GetGaugeFamily("process_virtual_memory_bytes", "Virtual memory size in bytes.", {});
  virtual_memory_bytes = &virtual_memory_bytes_family->Add({});
}

}  // namespace

std::once_flag init_flag;

std::vector<::prometheus::MetricFamily> Collect() {
  std::call_once(init_flag, InitProcessMetrics);
  UpdateProcessMetric();
  return collector->Collect();
}

}  // namespace trpc::prometheus
#endif
