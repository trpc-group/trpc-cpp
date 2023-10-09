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

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "trpc/runtime/common/heartbeat/heartbeat_info.h"
#include "trpc/runtime/threadmodel/common/worker_thread.h"
#include "trpc/util/function.h"
#include "trpc/util/queue/bounded_mpmc_queue.h"

namespace trpc {

/// @brief This singleton class is used for service and thread heartbeat reporting, it includes the following features:
///        1. Timely report the service heartbeat to the naming service.
///           Note: Use 'RegistryServiceHeartBeatInfo' interface to register the heartbeat information that needs to be
///           reported when the service is initialized. And then the heartbeat reporting task will periodically report
///           to the naming service though the function set by 'SetHeartBeatReportFunction' interface.
///        2. Detect whether the 'Handle' thread is dead in separate thread model during runtime.
///           Note: The 'Handle' thread reports its heartbeat every 3 seconds though 'ThreadHeartBeat' interface. The
///           heartbeat reporting task will periodically check whether the heartbeat has been reported within the
///           threshold. If it has not been reported for a long time, the thread is considered dead. When all 'Handle'
///           threads are detected to be dead, then the heartbeat reporting of the service will be stopped.
///        3. Report the task queue size of 'IO' or 'Handle' thread to the metrics system.
///           Note: The 'IO' or 'Handle' threads will report their own queue size in real-time through 'ThreadHeartBeat'
///           interface.
class HeartBeatReport {
 public:
   /// @private
  using ServiceRecvQueue = BoundedMPMCQueue<ServiceHeartBeatInfo>;
  /// @private
  using ThreadRecvQueue = BoundedMPMCQueue<ThreadHeartBeatInfo>;
  /// Function to report heartbeat to the naming service
  using ServiceReportFunction = Function<void(const std::string&, const ServiceHeartBeatInfo&)>;

  /// Function to report heartbeat information to monitor, only used to report queue size currently
  using ReportToMetricsFunction = Function<void(const ThreadHeartBeatInfo&)>;

  /// Used to store thread's heartbeat info by 'thread_id'
  /// @private
  using ThreadHandleIdMap = std::map<uint32_t, ThreadHeartBeatInfo>;
  /// Used to store all thread's heartbeat info by 'thread_role'
  /// @private
  using ThreadModelTypeMap = std::map<uint16_t, ThreadHandleIdMap>;
  /// Used to store all thread's heartbeat info of all thread model by 'group_name'
  /// @private
  using ThreadInstanceMap = std::map<std::string, ThreadModelTypeMap>;

 public:
  /// @brief Get singleton instance.
  static HeartBeatReport* GetInstance() {
    static HeartBeatReport instance;
    return &instance;
  }

  /// @private
  HeartBeatReport(const HeartBeatReport&) = delete;
  /// @private
  HeartBeatReport& operator=(const HeartBeatReport&) = delete;

  /// @private
  void Start();

  /// @private
  void Stop();

  /// @brief Framework use or for testing. Register heartbeat information of service.
  /// @private
  void RegisterServiceHeartBeatInfo(ServiceHeartBeatInfo&& service_heartbeat_info);

  /// @brief Framework use or for testing. Report heartbeat information of worker thread.
  /// @private
  void ThreadHeartBeat(ThreadHeartBeatInfo&& thread_heart_beat_info);

  /// @brief Used to set whether to perform server-level heartbeat reporting.
  /// @param report_switch true: reporting, false: not report
  void SetServerHeartBeatReportSwitch(bool report_switch);

  /// @brief Get value of server-level heartbeat reporting switch
  bool IsNeedReportServerHeartBeat();

  /// @brief Used to set whether to perform service-level heartbeat reporting.
  /// @param report_switch true: reporting, false: not report
  void SetServiceHeartBeatReportSwitch(const std::string& service_name, bool report_switch);

  /// @brief Get value of service-level heartbeat reporting switch by service name
  bool IsNeedReportServiceHeartBeat(const std::string& service_name);

  /// @brief Framework use or for testing. Set the related naming service for heartbeat reporting. 
  /// @private
  void SetRegistryName(std::string name) { registry_name_ = std::move(name); }

  /// @brief For testing. Get size of service-level heartbeat reporting queue
  /// @private
  uint32_t GetServiceQueueSize() {
    std::unique_lock<std::mutex> lock(service_mutex_);
    return static_cast<uint32_t>(service_recv_queue_.Size());
  }

  /// @brief For testing. Get size of worker thread's heartbeat reporting queue
  /// @private
  uint32_t GetThreadQueueSize() { return static_cast<uint32_t>(thread_recv_queue_.Size()); }

  /// @brief Set function that used for report heartbeat information to the naming service
  void SetHeartBeatReportFunction(ServiceReportFunction&& func) { report_function_ = std::move(func); }

  /// @brief Set function that used for report statistical data to the metrics service
  void SetReportMetricFunction(ReportToMetricsFunction&& func) { report_metrics_function_ = std::move(func); }

  /// @brief For testing. Set the time used to determine when a thread is deadlocked
  /// @private
  void SetThreadTimeOutSec(uint32_t time_sec) {
    if (time_sec > 0) thread_time_out_ms_ = time_sec * 1000;
  }

  /// @brief For testing. Get the number of thread model instances that all handle threads are deadlocked
  /// @private
  uint32_t GetIncompetentInstanceSize() const { return incompetent_instances_.size(); }

  /// @brief For testing. Set running state of heartbeat
  /// @private
  void SetRunning(bool running) { enable_ = running; }

 private:
  HeartBeatReport();

  void RecvThreadHeartBeat();

  uint32_t CheckThreadHeartBeat();

  void RecvServiceHeartBeatInfo();

  void AsyncHeartBeatForService();

  // Check if thread model is deadlocked
  bool CheckThreadModel(std::string const& instance_name) const {
    return incompetent_instances_.find(instance_name) == incompetent_instances_.end();
  }

  void Run();

  void DefaultReportFunction(const std::string& service_name, const ServiceHeartBeatInfo& heartbeat_info);

 private:
  bool enable_;

  uint64_t task_id_;

  uint64_t service_last_heartbeat_time_;

  uint64_t thread_last_heartbeat_time_;

  // the time used to determine when a thread is deadlocked
  uint64_t thread_time_out_ms_;

  std::unordered_map<std::string, ServiceHeartBeatInfo> service_heartbeat_infos_;

  ServiceRecvQueue service_recv_queue_;

  // used to protect service_recv_queue_
  std::mutex service_mutex_;

  // registry name of related naming service
  std::string registry_name_;

  uint32_t heartbeat_interval_;

  ServiceReportFunction report_function_{nullptr};

  ReportToMetricsFunction report_metrics_function_{nullptr};

  ThreadInstanceMap thread_heartbeat_info_;

  // thread model instances that all handle threads are deadlocked
  std::unordered_set<std::string> incompetent_instances_;

  ThreadRecvQueue thread_recv_queue_;

  // used to protect report_switch_ and service_heartbaet_switch_infos_ variables
  std::mutex report_switch_mutex_;

  // heartbeat reporting switch for service-level, the key is service name
  std::unordered_map<std::string, bool> service_heartbaet_switch_infos_;

  // heartbeat reporting switch for server-level
  bool report_switch_;
};

/// @private
/// @brief Get role of worker thread
inline std::string GetThreadRoleString(uint16_t thread_role) {
  switch (thread_role) {
    case kIo:
      return "IoModel";
    case kHandle:
      return "HandleModel";
    case kIoAndHandle:
      return "IoAndHandleModel";
    default:
      return "UNKNOWN";
  }
}

}  // namespace trpc
