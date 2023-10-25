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

#include "trpc/common/config/global_conf.h"

#include "trpc/util/log/logging.h"

namespace trpc {

void SeparateThreadModelSchedulingConfig::Display() const {
  TRPC_LOG_DEBUG("================================");

  TRPC_LOG_DEBUG("scheduling_name:" << scheduling_name);
  TRPC_LOG_DEBUG("local_queue_size:" << local_queue_size);
  TRPC_LOG_DEBUG("max_timer_size:" << max_timer_size);

  TRPC_LOG_DEBUG("================================");
}

void DefaultThreadModelInstanceConfig::Display() const {
  TRPC_LOG_DEBUG("================================");

  TRPC_LOG_DEBUG("instance_name:" << instance_name);
  TRPC_LOG_DEBUG("io_handle_type:" << io_handle_type);
  TRPC_LOG_DEBUG("io_thread_num:" << io_thread_num);
  TRPC_LOG_DEBUG("io_thread_task_queue_size:" << io_thread_task_queue_size);
  TRPC_LOG_DEBUG("io_cpu_affinitys:" << io_cpu_affinitys);
  TRPC_LOG_DEBUG("handle_thread_num:" << handle_thread_num);
  TRPC_LOG_DEBUG("handle_thread_task_queue_size:" << handle_thread_task_queue_size);
  TRPC_LOG_DEBUG("handle_cpu_affinitys:" << handle_cpu_affinitys);
  TRPC_LOG_DEBUG("disallow_cpu_migration:" << disallow_cpu_migration);
  TRPC_LOG_DEBUG("enable_async_io:" << enable_async_io);
  TRPC_LOG_DEBUG("io_uring_entries:" << io_uring_entries);
  TRPC_LOG_DEBUG("io_uring_flags:" << io_uring_flags);

  scheduling.Display();

  TRPC_LOG_DEBUG("================================");
}

void FiberThreadModelInstanceConfig::Display() const {
  TRPC_LOG_DEBUG("================================");

  TRPC_LOG_DEBUG("instance_name:" << instance_name);
  TRPC_LOG_DEBUG("concurrency_hint:" << concurrency_hint);
  TRPC_LOG_DEBUG("scheduling_group_size:" << scheduling_group_size);
  TRPC_LOG_DEBUG("numa_aware:" << numa_aware);
  TRPC_LOG_DEBUG("fiber_worker_accessible_cpus:" << fiber_worker_accessible_cpus);
  TRPC_LOG_DEBUG("fiber_worker_disallow_cpu_migration:" << fiber_worker_disallow_cpu_migration);
  TRPC_LOG_DEBUG("work_stealing_ratio:" << work_stealing_ratio);
  TRPC_LOG_DEBUG("reactor_num_per_scheduling_group:" << reactor_num_per_scheduling_group);
  TRPC_LOG_DEBUG("cross_numa_work_stealing_ratio:" << cross_numa_work_stealing_ratio);
  TRPC_LOG_DEBUG("fiber_run_queue_size:" << fiber_run_queue_size);
  TRPC_LOG_DEBUG("fiber_stack_size:" << fiber_stack_size);
  TRPC_LOG_DEBUG("fiber_pool_num_by_mmap:" << fiber_pool_num_by_mmap);
  TRPC_LOG_DEBUG("fiber_stack_enable_guard_page:" << fiber_stack_enable_guard_page);
  TRPC_LOG_DEBUG("fiber_scheduling_name:" << fiber_scheduling_name);
  TRPC_LOG_DEBUG("enable_gdb_debug:" << enable_gdb_debug);

  TRPC_LOG_DEBUG("================================");
}

void ThreadModelConfig::Display() const {
  TRPC_LOG_DEBUG("================================");

  for (const auto& i : fiber_model) {
    i.Display();
  }

  for (const auto& i : default_model) {
    i.Display();
  }

  TRPC_LOG_DEBUG("================================");
}

void HeartbeatConfig::Display() const {
  TRPC_LOG_DEBUG("================================");

  TRPC_LOG_DEBUG("enable_heartbeat:" << enable_heartbeat);
  TRPC_LOG_DEBUG("thread_heartbeat_time_out:" << thread_heartbeat_time_out);
  TRPC_LOG_DEBUG("heartbeat_report_interval:" << heartbeat_report_interval);

  TRPC_LOG_DEBUG("================================");
}

void BufferPoolConfig::Display() const {
  TRPC_LOG_DEBUG("================================");

  TRPC_LOG_DEBUG("mem_pool_threshold:" << mem_pool_threshold);
  TRPC_LOG_DEBUG("block_size:" << block_size);

  TRPC_LOG_DEBUG("================================");
}

void TvarConfig::Display() const {
  TRPC_LOG_DEBUG("================================");

  TRPC_LOG_DEBUG("window_size:" << window_size);
  TRPC_LOG_DEBUG("save_series:" << save_series);
  TRPC_LOG_DEBUG("abort_on_same_path:" << abort_on_same_path);
  TRPC_LOG_DEBUG("latency_p1:" << latency_p1);
  TRPC_LOG_DEBUG("latency_p2:" << latency_p2);
  TRPC_LOG_DEBUG("latency_p3:" << latency_p3);

  TRPC_LOG_DEBUG("================================");
}

void RpczConfig::Display() const {
  TRPC_LOG_DEBUG("================================");
  TRPC_LOG_DEBUG("lower_water_level:" << lower_water_level);
  TRPC_LOG_DEBUG("high_water_level:" << high_water_level);
  TRPC_LOG_DEBUG("sample_rate:" << sample_rate);
  TRPC_LOG_DEBUG("cache_expire_interval:" << cache_expire_interval);
  TRPC_LOG_DEBUG("collect_interval_ms:" << collect_interval_ms);
  TRPC_LOG_DEBUG("remove_interval_ms:" << remove_interval_ms);
  TRPC_LOG_DEBUG("print_spans_num:" << print_spans_num);

  TRPC_LOG_DEBUG("================================");
}

void GlobalConfig::Display() const {
  TRPC_LOG_DEBUG("=============global==============");

  TRPC_LOG_DEBUG("periphery_task_scheduler_thread_num:" << periphery_task_scheduler_thread_num);
  TRPC_LOG_DEBUG("inner_periphery_task_scheduler_thread_num:" << inner_periphery_task_scheduler_thread_num);
  TRPC_LOG_DEBUG("env_namespace:" << env_namespace);
  TRPC_LOG_DEBUG("env_name:" << env_name);
  TRPC_LOG_DEBUG("container_name:" << container_name);
  TRPC_LOG_DEBUG("local_ip:" << local_ip);
  TRPC_LOG_DEBUG("local_nic:" << local_nic);
  TRPC_LOG_DEBUG("enable_set:" << enable_set);
  TRPC_LOG_DEBUG("full_set_name:" << full_set_name);
  TRPC_LOG_DEBUG("thread_disable_process_name:" << thread_disable_process_name);

  threadmodel_config.Display();

  heartbeat_config.Display();

  tvar_config.Display();

  rpcz_config.Display();

  buffer_pool_config.Display();

  TRPC_LOG_DEBUG("=============global==============");
}

}  // namespace trpc
