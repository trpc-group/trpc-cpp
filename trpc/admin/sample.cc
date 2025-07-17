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

#include "trpc/admin/sample.h"

#include <stddef.h>

#include "trpc/util/log/logging.h"

namespace trpc::admin {

// loadavg
static PassiveMetrics<double, AverageOperation<double>>* proc_loadavg_1m = nullptr;
static PassiveMetrics<double, AverageOperation<double>>* proc_loadavg_5m = nullptr;
static PassiveMetrics<double, AverageOperation<double>>* proc_loadavg_15m = nullptr;

// proc usage
static PassiveMetrics<time_t, MaxOperation<time_t>>* proc_real_time = nullptr;
static PassiveMetrics<time_t, MaxOperation<time_t>>* proc_sys_time = nullptr;
static PassiveMetrics<time_t, MaxOperation<time_t>>* proc_user_time = nullptr;

// proc stat
static PassiveMetrics<uint64_t, AverageOperation<uint64_t>>* proc_faults_major = nullptr;
static PassiveMetrics<uint64_t, AverageOperation<uint64_t>>* proc_faults_minor_second = nullptr;

// proc io
static PassiveMetrics<size_t, AverageOperation<size_t>>* proc_io_read_bytes_second = nullptr;
static PassiveMetrics<size_t, AverageOperation<size_t>>* proc_io_read_second = nullptr;
static PassiveMetrics<size_t, AverageOperation<size_t>>* proc_io_write_bytes_second = nullptr;
static PassiveMetrics<size_t, AverageOperation<size_t>>* proc_io_write_second = nullptr;

// proc memory
static PassiveMetrics<int64_t, AverageOperation<int64_t>>* proc_mem_drs = nullptr;
static PassiveMetrics<int64_t, AverageOperation<int64_t>>* proc_mem_resident = nullptr;
static PassiveMetrics<int64_t, AverageOperation<int64_t>>* proc_mem_share = nullptr;
static PassiveMetrics<int64_t, AverageOperation<int64_t>>* proc_mem_trs = nullptr;
static PassiveMetrics<int64_t, AverageOperation<int64_t>>* proc_mem_size = nullptr;

void InitSystemVars() {
  {
    auto avg_op = new AverageOperation<double>();
    proc_loadavg_1m = new PassiveMetrics<double, AverageOperation<double>>("proc_loadavg_1m", avg_op);
  }

  {
    auto avg_op = new AverageOperation<double>();
    proc_loadavg_5m = new PassiveMetrics<double, AverageOperation<double>>("proc_loadavg_5m", avg_op);
  }

  {
    auto avg_op = new AverageOperation<double>();
    proc_loadavg_15m = new PassiveMetrics<double, AverageOperation<double>>("proc_loadavg_15m", avg_op);
  }

  {
    auto avg_op = new MaxOperation<time_t>();
    proc_real_time = new PassiveMetrics<time_t, MaxOperation<time_t>>("proc_real_time", avg_op);
  }

  {
    auto avg_op = new MaxOperation<time_t>();
    proc_sys_time = new PassiveMetrics<time_t, MaxOperation<time_t>>("proc_sys_time", avg_op);
  }

  {
    auto avg_op = new MaxOperation<time_t>();
    proc_user_time = new PassiveMetrics<time_t, MaxOperation<time_t>>("proc_user_time", avg_op);
  }

  {
    auto avg_op = new AverageOperation<uint64_t>();
    proc_faults_major = new PassiveMetrics<uint64_t, AverageOperation<uint64_t>>("proc_faults_major", avg_op);
  }

  {
    auto avg_op = new AverageOperation<uint64_t>();
    proc_faults_minor_second =
        new PassiveMetrics<uint64_t, AverageOperation<uint64_t>>("proc_faults_minor_second", avg_op);
  }

  {
    auto avg_op = new AverageOperation<size_t>();
    proc_io_read_bytes_second =
        new PassiveMetrics<size_t, AverageOperation<size_t>>("proc_io_read_bytes_second", avg_op);
  }

  {
    auto avg_op = new AverageOperation<size_t>();
    proc_io_read_second = new PassiveMetrics<size_t, AverageOperation<size_t>>("proc_io_read_second", avg_op);
  }

  {
    auto avg_op = new AverageOperation<size_t>();
    proc_io_write_bytes_second =
        new PassiveMetrics<size_t, AverageOperation<size_t>>("proc_io_write_bytes_second", avg_op);
  }

  {
    auto avg_op = new AverageOperation<size_t>();
    proc_io_write_second = new PassiveMetrics<size_t, AverageOperation<size_t>>("proc_io_write_second", avg_op);
  }

  {
    auto avg_op = new AverageOperation<int64_t>();
    proc_mem_drs = new PassiveMetrics<int64_t, AverageOperation<int64_t>>("proc_mem_drs", avg_op);
  }

  {
    auto avg_op = new AverageOperation<int64_t>();
    proc_mem_resident = new PassiveMetrics<int64_t, AverageOperation<int64_t>>("proc_mem_resident", avg_op);
  }

  {
    auto avg_op = new AverageOperation<int64_t>();
    proc_mem_share = new PassiveMetrics<int64_t, AverageOperation<int64_t>>("proc_mem_share", avg_op);
  }

  {
    auto avg_op = new AverageOperation<int64_t>();
    proc_mem_trs = new PassiveMetrics<int64_t, AverageOperation<int64_t>>("proc_mem_trs", avg_op);
  }

  {
    auto avg_op = new AverageOperation<int64_t>();
    proc_mem_size = new PassiveMetrics<int64_t, AverageOperation<int64_t>>("proc_mem_size", avg_op);
  }
}

void UpdateSystemVars() {
  LoadAverage load;
  if (ReadLoadUsage(&load)) {
    TRPC_LOG_ERROR("!!!, do ReadLoadUsage fail");
  } else {
    proc_loadavg_1m->TakeSample(load.loadavg_1m);
    proc_loadavg_5m->TakeSample(load.loadavg_5m);
    proc_loadavg_15m->TakeSample(load.loadavg_15m);
  }

  rusage stat;
  int ret = GetProcRusage(&stat);
  if (ret) {
    TRPC_LOG_ERROR("!!!, do GetProcRusage fail");
  } else {
    ProcRusage proc_usage;
    proc_usage.real_time = stat.ru_stime.tv_sec + stat.ru_utime.tv_sec;
    proc_usage.sys_time = stat.ru_stime.tv_sec;
    proc_usage.user_time = stat.ru_utime.tv_sec;

    proc_real_time->TakeSample(proc_usage.real_time);
    proc_sys_time->TakeSample(proc_usage.sys_time);
    proc_user_time->TakeSample(proc_usage.user_time);
  }

  // proc stat
  ProcStat pstat;
  ret = ReadProcState(&pstat);
  if (ret) {
    TRPC_LOG_ERROR("!!!, do ReadProcState fail");
  } else {
    proc_faults_major->TakeSample(pstat.majflt);
    proc_faults_minor_second->TakeSample(pstat.minflt);
  }

  // proc io
  ProcIO pio;
  ret = ReadProcIo(&pio);
  if (ret) {
    TRPC_LOG_ERROR("!!!, do ReadProcIo fail");
  } else {
    proc_io_read_bytes_second->TakeSample(pio.read_bytes);
    proc_io_read_second->TakeSample(pio.syscr);
    proc_io_write_bytes_second->TakeSample(pio.write_bytes);
    proc_io_write_second->TakeSample(pio.syscw);
  }

  // proc memory
  ProcMemory pmem;
  ret = ReadProcMemory(&pmem);
  if (ret) {
    TRPC_LOG_ERROR("!!!, do ReadProcMemory fail");
  } else {
    proc_mem_drs->TakeSample(pmem.drs);
    proc_mem_resident->TakeSample(pmem.resident);
    proc_mem_share->TakeSample(pmem.share);
    proc_mem_trs->TakeSample(pmem.trs);
    proc_mem_size->TakeSample(pmem.size);
  }
}

void GetVarSeriesData(const std::string var_name, rapidjson::Value& result, rapidjson::Document::AllocatorType& alloc) {
  auto metric_var = MonitorVarsMap::GetInstance()->GetMonitorMetrics(var_name);
  if (metric_var) {
    metric_var->GetHistoryData(result, alloc);
  }
}

}  // namespace trpc::admin
