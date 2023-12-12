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

#include <map>
#include <string>
#include <vector>

#include "yaml-cpp/yaml.h"

#include "trpc/common/config/global_conf.h"
#include "trpc/util/net_util.h"

namespace YAML {

template <>
struct convert<trpc::FiberThreadModelInstanceConfig> {
  static YAML::Node encode(const trpc::FiberThreadModelInstanceConfig& config) {
    YAML::Node node;

    node["instance_name"] = config.instance_name;
    node["concurrency_hint"] = config.concurrency_hint;
    node["scheduling_group_size"] = config.scheduling_group_size;
    node["numa_aware"] = config.numa_aware;
    node["fiber_worker_accessible_cpus"] = config.fiber_worker_accessible_cpus;
    node["fiber_worker_disallow_cpu_migration"] = config.fiber_worker_disallow_cpu_migration;
    node["work_stealing_ratio"] = config.work_stealing_ratio;
    node["reactor_num_per_scheduling_group"] = config.reactor_num_per_scheduling_group;
    node["reactor_task_queue_size"] = config.reactor_task_queue_size;
    node["cross_numa_work_stealing_ratio"] = config.cross_numa_work_stealing_ratio;
    node["fiber_run_queue_size"] = config.fiber_run_queue_size;
    node["fiber_stack_size"] = config.fiber_stack_size;
    node["fiber_pool_num_by_mmap"] = config.fiber_pool_num_by_mmap;
    node["fiber_stack_enable_guard_page"] = config.fiber_stack_enable_guard_page;
    node["fiber_scheduling_name"] = config.fiber_scheduling_name;
    node["enable_gdb_debug"] = config.enable_gdb_debug;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::FiberThreadModelInstanceConfig& config) {
    if (node["instance_name"]) {
      config.instance_name = node["instance_name"].as<std::string>();
    }

    if (node["concurrency_hint"]) {
      config.concurrency_hint = node["concurrency_hint"].as<uint32_t>();
    }

    if (node["scheduling_group_size"]) {
      config.scheduling_group_size = node["scheduling_group_size"].as<uint32_t>();
    }

    if (node["numa_aware"]) {
      config.numa_aware = node["numa_aware"].as<bool>();
    }

    if (node["fiber_worker_accessible_cpus"]) {
      config.fiber_worker_accessible_cpus = node["fiber_worker_accessible_cpus"].as<std::string>();
    }

    if (node["fiber_worker_disallow_cpu_migration"]) {
      config.fiber_worker_disallow_cpu_migration = node["fiber_worker_disallow_cpu_migration"].as<bool>();
    }

    if (node["work_stealing_ratio"]) {
      config.work_stealing_ratio = node["work_stealing_ratio"].as<uint32_t>();
    }

    if (node["reactor_num_per_scheduling_group"]) {
      config.reactor_num_per_scheduling_group = node["reactor_num_per_scheduling_group"].as<uint32_t>();
    }

    if (node["reactor_task_queue_size"]) {
      config.reactor_task_queue_size = node["reactor_task_queue_size"].as<uint32_t>();
    }

    if (node["cross_numa_work_stealing_ratio"]) {
      config.cross_numa_work_stealing_ratio = node["cross_numa_work_stealing_ratio"].as<uint32_t>();
    }

    if (node["fiber_run_queue_size"]) {
      config.fiber_run_queue_size = node["fiber_run_queue_size"].as<uint32_t>();
    }

    if (node["fiber_stack_size"]) {
      config.fiber_stack_size = node["fiber_stack_size"].as<uint32_t>();
    }

    if (node["fiber_pool_num_by_mmap"]) {
      config.fiber_pool_num_by_mmap = node["fiber_pool_num_by_mmap"].as<uint32_t>();
    }

    if (node["fiber_stack_enable_guard_page"]) {
      config.fiber_stack_enable_guard_page = node["fiber_stack_enable_guard_page"].as<bool>();
    }

    if (node["fiber_scheduling_name"]) {
      config.fiber_scheduling_name = node["fiber_scheduling_name"].as<std::string>();
    }

    if (config.fiber_scheduling_name.empty()) {
      config.fiber_scheduling_name = "v1";
    }

    if (node["enable_gdb_debug"]) {
      config.enable_gdb_debug = node["enable_gdb_debug"].as<bool>();
    }

    return true;
  }
};

template <>
struct convert<trpc::SeparateThreadModelSchedulingConfig> {
  static YAML::Node encode(const trpc::SeparateThreadModelSchedulingConfig& config) {
    YAML::Node node;

    node["scheduling_name"] = config.scheduling_name;
    node["local_queue_size"] = config.local_queue_size;
    node["max_timer_size"] = config.max_timer_size;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::SeparateThreadModelSchedulingConfig& config) {
    if (node["scheduling_name"]) {
      config.scheduling_name = node["scheduling_name"].as<std::string>();
      // for compatible
      if (config.scheduling_name == "non_fiber") {
        config.scheduling_name = "non_steal";
      } else if (config.scheduling_name == "task_steal") {
        config.scheduling_name = "steal";
      }
    }

    if (node["local_queue_size"]) {
      config.local_queue_size = node["local_queue_size"].as<uint32_t>();
    }

    if (node["max_timer_size"]) {
      config.max_timer_size = node["max_timer_size"].as<uint32_t>();
    }

    return true;
  }
};

template <>
struct convert<trpc::DefaultThreadModelInstanceConfig> {
  static YAML::Node encode(const trpc::DefaultThreadModelInstanceConfig& config) {
    YAML::Node node;

    node["instance_name"] = config.instance_name;
    node["io_handle_type"] = config.io_handle_type;
    node["io_thread_num"] = config.io_thread_num;
    node["io_cpu_affinitys"] = config.io_cpu_affinitys;
    node["handle_thread_num"] = config.handle_thread_num;
    node["handle_cpu_affinitys"] = config.handle_cpu_affinitys;
    node["disallow_cpu_migration"] = config.disallow_cpu_migration;
    node["io_thread_task_queue_size"] = config.io_thread_task_queue_size;
    node["handle_thread_task_queue_size"] = config.handle_thread_task_queue_size;
    node["scheduling"] = config.scheduling;
    node["enable_async_io"] = config.enable_async_io;
    node["io_uring_entries"] = config.io_uring_entries;
    node["io_uring_flags"] = config.io_uring_flags;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::DefaultThreadModelInstanceConfig& config) {
    if (node["instance_name"]) {
      config.instance_name = node["instance_name"].as<std::string>();
    }

    if (node["io_handle_type"]) {
      config.io_handle_type = node["io_handle_type"].as<std::string>();
    }

    if (node["io_thread_num"]) {
      config.io_thread_num = node["io_thread_num"].as<uint32_t>();
    }

    if (node["io_cpu_affinitys"]) {
      config.io_cpu_affinitys = node["io_cpu_affinitys"].as<std::string>();
    }

    if (node["handle_thread_num"]) {
      config.handle_thread_num = node["handle_thread_num"].as<uint32_t>();
    }

    if (node["handle_cpu_affinitys"]) {
      config.handle_cpu_affinitys = node["handle_cpu_affinitys"].as<std::string>();
    }

    if (node["disallow_cpu_migration"]) {
      config.disallow_cpu_migration = node["disallow_cpu_migration"].as<bool>();
    }

    if (node["io_thread_task_queue_size"]) {
      config.io_thread_task_queue_size = node["io_thread_task_queue_size"].as<uint32_t>();
    }

    if (node["handle_thread_task_queue_size"]) {
      config.handle_thread_task_queue_size = node["handle_thread_task_queue_size"].as<uint32_t>();
    }

    if (node["scheduling"]) {
      config.scheduling = node["scheduling"].as<trpc::SeparateThreadModelSchedulingConfig>();
    }

    if (node["enable_async_io"]) {
      config.enable_async_io = node["enable_async_io"].as<bool>();
    }

    if (node["io_uring_entries"]) {
      config.io_uring_entries = node["io_uring_entries"].as<uint32_t>();
    }

    if (node["io_uring_flags"]) {
      config.io_uring_flags = node["io_uring_flags"].as<uint32_t>();
    }

    return true;
  }
};


template <>
struct convert<trpc::HeartbeatConfig> {
  static YAML::Node encode(const trpc::HeartbeatConfig& config) {
    YAML::Node node;
    node["enable_heartbeat"] = config.enable_heartbeat;
    node["thread_heartbeat_time_out"] = config.thread_heartbeat_time_out;
    node["heartbeat_report_interval"] = config.heartbeat_report_interval;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::HeartbeatConfig& config) {
    if (node["enable_heartbeat"]) {
      config.enable_heartbeat = node["enable_heartbeat"].as<bool>();
    }
    if (node["thread_heartbeat_time_out"]) {
      config.thread_heartbeat_time_out = node["thread_heartbeat_time_out"].as<uint32_t>();
    }
    if (node["heartbeat_report_interval"]) {
      config.heartbeat_report_interval = node["heartbeat_report_interval"].as<uint32_t>();
    }

    return true;
  }
};

template <>
struct convert<trpc::BufferPoolConfig> {
  static YAML::Node encode(const trpc::BufferPoolConfig& config) {
    YAML::Node node;
    node["mem_pool_threshold"] = config.mem_pool_threshold;
    node["block_size"] = config.block_size;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::BufferPoolConfig& config) {
    if (node["mem_pool_threshold"]) {
      config.mem_pool_threshold = node["mem_pool_threshold"].as<uint32_t>();
    }
    if (node["block_size"]) {
      config.block_size = node["block_size"].as<uint32_t>();
    }
    return true;
  }
};

template <>
struct convert<trpc::TvarConfig> {
  static YAML::Node encode(const trpc::TvarConfig& config) {
    YAML::Node node;
    node["window_size"] = config.window_size;
    node["save_series"] = config.save_series;
    node["abort_on_same_path"] = config.abort_on_same_path;
    node["latency_p1"] = config.latency_p1;
    node["latency_p2"] = config.latency_p2;
    node["latency_p3"] = config.latency_p3;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::TvarConfig& config) {
    if (node["window_size"]) {
      config.window_size = node["window_size"].as<uint32_t>();
    }
    if (node["save_series"]) {
      config.save_series = node["save_series"].as<bool>();
    }
    if (node["abort_on_same_path"]) {
      config.abort_on_same_path = node["abort_on_same_path"].as<bool>();
    }
    if (node["latency_p1"]) {
      config.latency_p1 = node["latency_p1"].as<uint32_t>();
    }
    if (node["latency_p2"]) {
      config.latency_p2 = node["latency_p2"].as<uint32_t>();
    }
    if (node["latency_p3"]) {
      config.latency_p3 = node["latency_p3"].as<uint32_t>();
    }

    return true;
  }
};

template <>
struct convert<trpc::RpczConfig> {
  static YAML::Node encode(const trpc::RpczConfig& config) {
    YAML::Node node;
    node["lower_water_level"] = config.lower_water_level;
    node["high_water_level"] = config.high_water_level;
    node["sample_rate"] = config.sample_rate;
    node["cache_expire_interval"] = config.cache_expire_interval;
    node["collect_interval_ms"] = config.collect_interval_ms;
    node["remove_interval_ms"] = config.remove_interval_ms;
    node["print_spans_num"] = config.print_spans_num;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::RpczConfig& config) {
    if (node["lower_water_level"]) {
      config.lower_water_level = node["lower_water_level"].as<uint32_t>();
    }
    if (node["high_water_level"]) {
      config.high_water_level = node["high_water_level"].as<uint32_t>();
    }
    if (node["sample_rate"]) {
      config.sample_rate = node["sample_rate"].as<uint32_t>();
    }
    if (node["cache_expire_interval"]) {
      config.cache_expire_interval = node["cache_expire_interval"].as<uint32_t>();
    }
    if (node["collect_interval_ms"]) {
      config.collect_interval_ms = node["collect_interval_ms"].as<uint32_t>();
    }
    if (node["remove_interval_ms"]) {
      config.remove_interval_ms = node["remove_interval_ms"].as<uint32_t>();
    }
    if (node["print_spans_num"]) {
      config.print_spans_num = node["print_spans_num"].as<uint32_t>();
    }

    return true;
  }
};

template <>
struct convert<trpc::ThreadModelConfig> {
  static YAML::Node encode(const trpc::ThreadModelConfig& config) {
    YAML::Node node;

    node["fiber"] = config.fiber_model;
    node["default"] = config.default_model;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::ThreadModelConfig& config) {
    if (node["fiber"]) {
      config.use_fiber_flag = true;
      for (size_t idx = 0; idx < node["fiber"].size(); ++idx) {
        auto item = node["fiber"][idx].as<trpc::FiberThreadModelInstanceConfig>();
        config.fiber_model.push_back(item);
      }
    }

    if (node["default"]) {
      for (size_t idx = 0; idx < node["default"].size(); ++idx) {
        auto item = node["default"][idx].as<trpc::DefaultThreadModelInstanceConfig>();
        config.default_model.push_back(item);
      }
    }

    return true;
  }
};

template <>
struct convert<trpc::GlobalConfig> {
  static YAML::Node encode(const trpc::GlobalConfig& global_config) {
    YAML::Node node;

    node["local_ip"] = global_config.local_ip;
    node["local_nic"] = global_config.local_nic;
    node["namespace"] = global_config.env_namespace;
    node["env_name"] = global_config.env_name;
    node["container_name"] = global_config.container_name;
    node["full_set_name"] = global_config.full_set_name;
    node["enable_set"] = global_config.enable_set ? "Y" : "N";
    node["thread_disable_process_name"] = global_config.thread_disable_process_name;
    node["periphery_task_scheduler_thread_num"] = global_config.periphery_task_scheduler_thread_num;
    node["inner_periphery_task_scheduler_thread_num"] = global_config.inner_periphery_task_scheduler_thread_num;
    node["enable_runtime_report"] = global_config.enable_runtime_report;
    node["report_runtime_info_interval"] = global_config.report_runtime_info_interval;
    node["threadmodel"] = global_config.threadmodel_config;
    node["heartbeat"] = global_config.heartbeat_config;
    node["buffer_pool"] = global_config.buffer_pool_config;
    node["tvar"] = global_config.tvar_config;
    node["rpcz"] = global_config.rpcz_config;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::GlobalConfig& global_config) {
    if (node["local_ip"]) {
      global_config.local_ip = node["local_ip"].as<std::string>();
    } else if (!global_config.local_nic.empty()) {
      global_config.local_ip = trpc::util::GetIpByEth(global_config.local_nic);
    }

    if (node["local_nic"]) {
        global_config.local_nic = node["local_nic"].as<std::string>();
    }

    if (node["namespace"]) {
        global_config.env_namespace = node["namespace"].as<std::string>();
    }

    if (node["env_name"]) {
        global_config.env_name = node["env_name"].as<std::string>();
    }

    if (node["container_name"]) {
        global_config.container_name = node["container_name"].as<std::string>();
    }

    if (node["full_set_name"]) {
      global_config.full_set_name =
          (YAML::NodeType::Null == node["full_set_name"].Type()) ? "" : node["full_set_name"].as<std::string>();
    }

    if (node["enable_set"]) {
      std::string str_enable_set =
          (YAML::NodeType::Null == node["enable_set"].Type()) ? "" : node["enable_set"].as<std::string>();
      global_config.enable_set = (str_enable_set == "Y");
    }

    if (node["thread_disable_process_name"]) {
      global_config.thread_disable_process_name = node["thread_disable_process_name"].as<bool>();
    }

    if (node["periphery_task_scheduler_thread_num"]) {
      global_config.periphery_task_scheduler_thread_num = node["periphery_task_scheduler_thread_num"].as<uint32_t>();
    }

    if (node["inner_periphery_task_scheduler_thread_num"]) {
      global_config.inner_periphery_task_scheduler_thread_num =
          node["inner_periphery_task_scheduler_thread_num"].as<uint32_t>();
    }

    if (node["enable_runtime_report"]) {
      global_config.enable_runtime_report = node["enable_runtime_report"].as<bool>();
    }

    if (node["report_runtime_info_interval"]) {
      global_config.report_runtime_info_interval = node["report_runtime_info_interval"].as<uint32_t>();
    }

    if (node["threadmodel"]) {
      auto item = node["threadmodel"].as<trpc::ThreadModelConfig>();
      global_config.threadmodel_config = item;
    }

    if (node["heartbeat"]) {
      global_config.heartbeat_config = node["heartbeat"].as<trpc::HeartbeatConfig>();
    }

    if (node["buffer_pool"]) {
      global_config.buffer_pool_config = node["buffer_pool"].as<trpc::BufferPoolConfig>();
    }

    if (node["tvar"]) {
      global_config.tvar_config = node["tvar"].as<trpc::TvarConfig>();
    }

    if (node["rpcz"]) {
      global_config.rpcz_config = node["rpcz"].as<trpc::RpczConfig>();
    }

    return true;
  }
};

}  // namespace YAML
