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

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace trpc {

/// @brief Fiber(m:n coroutine) threadmodel instance config
struct FiberThreadModelInstanceConfig {
  /// @brief Fiber threadmodel instance name
  std::string instance_name;

  /// @brief The number of fiber worker thread
  /// @note  Recommend to set the same number as the number of cpu cores
  /// if not set, the value will reads from `/proc/cpuinfo` by framework
  uint32_t concurrency_hint{0};

  /// @brief The number of fiber worker threads per scheduling group
  /// @note  If you do not want to configure multiple scheduling groups
  /// the value set the same as `concurrency_hint`
  /// if this value is not set, framework will automatically set reasonable value
  uint32_t scheduling_group_size{0};

  /// @brief Whether to support numa cpu architecture
  bool numa_aware{false};

  /// @brief If set, each fiber worker is bound to exactly one cpu core, and each core
  /// is dedicated to exactly one fiber worker.
  /// `concurrency_hint` (if set) must be equal to the number of CPUs in the system (or in case
  /// `fiber_worker_accessible_cpus` is set as well, the number of CPUs accessible to fiber worker.)
  /// incorrect use of this option can actually hurt performance
  bool fiber_worker_disallow_cpu_migration{false};

  /// @brief If set, fiber workers only run on the cpus given
  /// @note cpus are specified in range or cpu IDs, e.g.: 0-7,8,9
  std::string fiber_worker_accessible_cpus;

  /// @brief Name of fiber scheduler implementation
  /// two scheduler implementations are currently supported
  /// v1: global queue
  /// v2: global queue + local queue, for taskflow
  std::string fiber_scheduling_name;

  /// @brief The ratio of stealing job between different scheduling group in same numa domain
  /// only use in scheduling v1
  uint32_t work_stealing_ratio{16};

  /// @brief Same as `work_stealing_ratio`
  /// but applied to stealing work from scheduling groups belonging to different numa domain.
  /// only use in scheduling v1
  uint32_t cross_numa_work_stealing_ratio{0};

  /// @brief The number of fiber reactors per scheduler
  uint32_t reactor_num_per_scheduling_group{0};

  /// @brief The size of fiber reactor task queue
  uint32_t reactor_task_queue_size{65536};

  /// @brief The size of fiber running queue
  uint32_t fiber_run_queue_size{131072};

  /// @brief The size of fiber memory stack
  uint32_t fiber_stack_size{131072};

  /// @brief The number of fiber stack created by mmap
  /// if exceed, use by malloc
  uint32_t fiber_pool_num_by_mmap{30 * 1024};

  /// @brief Stack overflow protect
  bool fiber_stack_enable_guard_page{true};

  /// @brief Enable debug fiber using gdb
  bool enable_gdb_debug = false;

  void Display() const;
};

/// @brief Separate threadmodel logic task scheduling config
struct SeparateThreadModelSchedulingConfig {
  /// @brief The name of scheduling for business logic task
  std::string scheduling_name;

  /// @brief The size of private task queue each handle thread
  uint32_t local_queue_size{65536};

  /// @brief Max size of timer each handle thread
  uint32_t max_timer_size{10240};

  void Display() const;
};

/// @brief Io/handle merge/separate threadmodel instance config
struct DefaultThreadModelInstanceConfig {
  /// @brief Io/handle merge/separate threadmodel instance name
  /// @note  Ensure uniqueness when existed multiple instances
  std::string instance_name;

  /// @brief Io and handle merge or separate
  std::string io_handle_type;

  /// @brief Network io thread num
  /// @note  Must be greater than or equal to 1
  uint32_t io_thread_num{1};

  /// @brief The size of network io thread task queue
  uint32_t io_thread_task_queue_size{65536};

  /// @brief Network io thread cpu affinitys
  std::string io_cpu_affinitys;

  /// @brief Business logic hanlde thread num
  uint32_t handle_thread_num{0};

  /// @brief The size of handle thread task queue
  uint32_t handle_thread_task_queue_size{65536};

  /// @brief Business logic hanlde thread cpu affinitys
  std::string handle_cpu_affinitys;

  /// @brief Whether to allow migration on multiple cpu cores
  /// @note  True：one thread bind one cpu,
  ///        False: one thread and one cpu no relationship
  ///        In separate threadmodel, handle_cpu_affinitys and io_cpu_affinitys need be configured
  ///        In merge threadmodel, io_cpu_affinitys need be configured
  bool disallow_cpu_migration{false};

  /// @brief For separate threadmodel
  SeparateThreadModelSchedulingConfig scheduling;

  /// @brief Whether to use async_io
  bool enable_async_io{false};

  /// @brief Io_uring queue size
  uint32_t io_uring_entries{1024};

  /// @brief Io_uring initilize flag
  uint32_t io_uring_flags{0};

  void Display() const;
};

/// @brief Framework threadmodel configuration
/// @note  Under normal circumstances, you should choose one threadmodel
/// If you want to use multi threadmodel, suggest you contact the trpc technical team
struct ThreadModelConfig {
  /// @brief Whether to use fiber model
  bool use_fiber_flag{false};

  /// @brief For fiber, only configure one instance
  std::vector<FiberThreadModelInstanceConfig> fiber_model;

  /// @brief For merge/separate threadmodel
  std::vector<DefaultThreadModelInstanceConfig> default_model;

  void Display() const;
};

/// @brief Related configuration of heart-beat within the framework
struct HeartbeatConfig {
  /// @brief Whether to enable the ability of heart-beat
  /// @note  If set false, Will not regularly report heartbeat to name service,
  /// Will not detect thread zombies and reported the size of task-queue(only in separate/merge threadmodel).
  bool enable_heartbeat{true};

  /// @brief The timeout(ms) of detact thread zombies
  /// @note  If the thread is zombies, it will no longer report heartbeat to the name service
  /// Default value is 60000(ms), If the value smaller than 30000(ms), it will be reset to 30000(ms)
  uint32_t thread_heartbeat_time_out{60000};

  /// @brief Hearbeat reporting interval in ms
  uint32_t heartbeat_report_interval{3000};

  void Display() const;
};

/// @brief Related configuration of buffer memory pool within the framework
struct BufferPoolConfig {
  /// @brief Memory pool size threshold
  /// If it exceeds, the buffer memory is directly allocated from the system
  uint32_t mem_pool_threshold = 512 * 1024 * 1024;

  /// @brief The size of each buffer memory block
  uint32_t block_size = 4096;

  void Display() const;
};

/// @brief Configurations for tvar.
/// @note p999 and p9999 will be recorded by default, still open three percentages to users.
struct TvarConfig {
  /// @brief Size of window, for Window and PerSecond.
  uint32_t window_size{10};
  /// @brief Whether to save history data.
  bool save_series{true};
  /// @brief Whether to abort if encounter two same exposed paths.
  bool abort_on_same_path{true};
  /// @brief First percentage, must resides in [1, 99].
  uint32_t latency_p1{80};
  /// @brief Second percentage, must resides in [1, 99].
  uint32_t latency_p2{90};
  /// @brief third percentage, must resides in [1, 99].
  uint32_t latency_p3{99};

  void Display() const;
};

/// @brief Configurations for rpcz.
struct RpczConfig {
  /// @brief Sample count less than lower_water_level, all sampled.
  uint32_t lower_water_level{500};
  /// @brief Sample count more than high_water_level, do probability sampled.
  uint32_t high_water_level{1000};
  /// @brief Sample rate when do probability sampled.
  uint32_t sample_rate{50};

  /// @brief Cached time of rpcz data before expired from memory, unit/second,
  /// @note Must bigger than or equal to 10, if less than 10, will be set to 10 to improve performance.
  uint32_t cache_expire_interval{10};

  /// @brief How long does rpcz thread collect thread-local data in milliseconds.
  uint32_t collect_interval_ms{500};

  /// @brief How long does rpcz thread clean thread-local data in milliseconds.
  uint32_t remove_interval_ms{5000};

  /// @brief How many spans to returned to admin request.
  uint32_t print_spans_num{10};

  void Display() const;
};

/// @brief Framework global configuration information
struct GlobalConfig {
  /// @brief [optional] Local ip
  std::string local_ip;

  /// @brief Nic name
  /// Use `local_ip` first, if local_ip is empty, use it
  std::string local_nic;

  /// @brief Namespace, used by naming plugin
  std::string env_namespace;

  /// @brief Env name, used by naming plugin
  std::string env_name;

  /// @brief Container name, used by metrics plugin
  std::string container_name;

  /// @brief Set full name
  /// Format is: set-name.set-area.set-group
  std::string full_set_name;

  /// @brief Whether to enable the ability that deploy by set
  bool enable_set = false;

  /// @brief The thread is whether to display the original name of the process
  /// @note  Default display the custom name
  bool thread_disable_process_name{true};

  /// @brief The number of threads executing business timing tasks
  uint32_t periphery_task_scheduler_thread_num{1};

  /// @brief The number of threads executing framework timing tasks
  uint32_t inner_periphery_task_scheduler_thread_num{2};

  /// @brief Whether to enable do the runtime info reporting
  bool enable_runtime_report{true};

  /// @brief periodic interval of the runtime info reporting in milliseconds
  uint32_t report_runtime_info_interval{60000};

  /// @brief Framework threadmodel config
  /// @note  Choose one threadmodel to use
  ThreadModelConfig threadmodel_config;

  /// @brief Related configuration of thread heartbeat reporting within the framework
  HeartbeatConfig heartbeat_config;

  /// @brief Related configuration of buffer pool within the framework
  BufferPoolConfig buffer_pool_config;

  /// @brief Tvar config
  TvarConfig tvar_config;

  /// @brief Rpcz config
  RpczConfig rpcz_config;

  void Display() const;
};

}  // namespace trpc
