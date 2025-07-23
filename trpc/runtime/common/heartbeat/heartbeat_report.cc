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

#include "trpc/runtime/common/heartbeat/heartbeat_report.h"

#include "fmt/format.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/naming/trpc_naming.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc {

namespace {

// Check if all handle threads are deadlocked
bool CheckModelTypeIncompetent(const trpc::HeartBeatReport::ThreadHandleIdMap& handle_id_map, uint64_t time_out_ms,
                               uint32_t* time_out_count) {
  if (handle_id_map.empty()) {
    return false;
  }

  bool incompetent = true;
  uint64_t now_ms = trpc::time::GetMilliSeconds();
  for (auto const& kv : handle_id_map) {
    auto const& heart_beat_info = kv.second;
    if (now_ms >= heart_beat_info.last_time_ms + time_out_ms) {
      ++(*time_out_count);
      TRPC_FMT_ERROR(
          "ThreadHeartBeat TimeOut. {}, {}, {}, {}, last_time:{}, time_now:{}, time_diff:{}, "
          "time_out:{}",
          heart_beat_info.group_name, GetThreadRoleString(heart_beat_info.role), heart_beat_info.id,
          heart_beat_info.pid, heart_beat_info.last_time_ms, now_ms, now_ms - heart_beat_info.last_time_ms,
          time_out_ms);
    } else {
      // If one handle thread is alive, it will not be considered available.
      incompetent = false;
    }
  }

  return incompetent;
}

}  // namespace

// The time interval used to check the thread status
constexpr uint64_t kThreadRecvIntervalMs = 5000;

// the minimum time used to determine when a thread is deadlocked
constexpr uint64_t kMinHeartbeatTimeOutMs = 30000;

HeartBeatReport::HeartBeatReport()
    : enable_(false),
      task_id_(0),
      service_last_heartbeat_time_(0),
      thread_last_heartbeat_time_(0),
      thread_time_out_ms_(0),
      report_switch_(true) {
  TRPC_ASSERT(service_recv_queue_.Init(1024));
  TRPC_ASSERT(thread_recv_queue_.Init(1024));

  report_function_ =
      std::bind(&HeartBeatReport::DefaultReportFunction, this, std::placeholders::_1, std::placeholders::_2);

  RegisterThreadHeartBeatFunction(
      [this](ThreadHeartBeatInfo&& heartbeat_info) { ThreadHeartBeat(std::move(heartbeat_info)); });

  const GlobalConfig& global_config = TrpcConfig::GetInstance()->GetGlobalConfig();
  if (!global_config.heartbeat_config.enable_heartbeat) {
    return;
  }

  thread_time_out_ms_ = global_config.heartbeat_config.thread_heartbeat_time_out;
  if (thread_time_out_ms_ < kMinHeartbeatTimeOutMs) {
    thread_time_out_ms_ = kMinHeartbeatTimeOutMs;
  }
  TRPC_FMT_TRACE("thread_time_out_ms_:{}", thread_time_out_ms_);

  registry_name_ = TrpcConfig::GetInstance()->GetServerConfig().registry_name;
  TRPC_LOG_TRACE("registry name: " << registry_name_);
  if (registry_name_.empty()) {
    TRPC_LOG_DEBUG("registry name empty, service heartbeat will not work.");
    return;
  }

  heartbeat_interval_ = TrpcConfig::GetInstance()->GetGlobalConfig().heartbeat_config.heartbeat_report_interval;
}

void HeartBeatReport::Start() {
  if (task_id_ != 0) {
    return;
  }

  task_id_ = PeripheryTaskScheduler::GetInstance()->SubmitInnerPeriodicalTask(std::bind(&HeartBeatReport::Run, this),
                                                                              1000, "HeartBeatReport");
  if (task_id_ > 0) {
    enable_ = true;
  }
}

void HeartBeatReport::Stop() {
  if (task_id_ == 0) {
    return;
  }

  enable_ = false;

  PeripheryTaskScheduler::GetInstance()->StopInnerTask(task_id_);
  PeripheryTaskScheduler::GetInstance()->JoinInnerTask(task_id_);

  task_id_ = 0;
}

void HeartBeatReport::RegisterServiceHeartBeatInfo(ServiceHeartBeatInfo&& service_heartbeat_info) {
  if (!enable_) {
    return;
  }

  if (!service_recv_queue_.Push(std::move(service_heartbeat_info))) {
    TRPC_LOG_ERROR("service heart-beat report queue is full");
  }
}

void HeartBeatReport::RecvServiceHeartBeatInfo() {
  if (registry_name_.empty()) {
    return;
  }

  uint32_t recv_count = 0;
  ServiceHeartBeatInfo info;
  while (service_recv_queue_.Pop(info)) {
    if (service_heartbeat_infos_.find(info.service_name) == service_heartbeat_infos_.end()) {
      service_heartbeat_infos_[info.service_name] = info;
    }
    ++recv_count;
  }

  TRPC_FMT_TRACE("ServiceHeartBeat recv count:{}", recv_count);
}

void HeartBeatReport::ThreadHeartBeat(ThreadHeartBeatInfo&& thread_heart_beat_info) {
  if (!enable_) {
    return;
  }

  // report some thread heartbeat information to the metrics service, such as task queue size
  if (report_metrics_function_) {
    report_metrics_function_(thread_heart_beat_info);
  }

  if (!thread_recv_queue_.Push(std::move(thread_heart_beat_info))) {
    TRPC_LOG_ERROR("thread heart-beat report queue is full");
  }
}

void HeartBeatReport::RecvThreadHeartBeat() {
  uint32_t recv_count = 0;
  ThreadHeartBeatInfo info;
  while (thread_recv_queue_.Pop(info)) {
    TRPC_FMT_TRACE("ThreadHeartBeat {}, {}, {}, {}, {}", info.group_name, GetThreadRoleString(info.role), info.id,
                   info.pid, info.last_time_ms);
    thread_heartbeat_info_[info.group_name][info.role][info.id] = info;
    ++recv_count;
  }

  TRPC_FMT_TRACE("ThreadHeartBeat info count:{}", recv_count);
}

void HeartBeatReport::Run() {
  RecvServiceHeartBeatInfo();
  RecvThreadHeartBeat();

  uint64_t now_ms = trpc::time::GetMilliSeconds();
  // report service heartbeat to the naming service regularly
  if (now_ms >= service_last_heartbeat_time_ + heartbeat_interval_) {
    AsyncHeartBeatForService();
    service_last_heartbeat_time_ = now_ms;
  }

  // check deadlock regularly
  if (now_ms >= thread_last_heartbeat_time_ + kThreadRecvIntervalMs) {
    CheckThreadHeartBeat();
    thread_last_heartbeat_time_ = now_ms;
  }
}

void HeartBeatReport::AsyncHeartBeatForService() {
  if (registry_name_.empty()) {
    return;
  }

  uint32_t report_count = 0;
  for (auto const& info_kv : service_heartbeat_infos_) {
    auto const& service_name = info_kv.first;
    auto const& group_name = info_kv.second.group_name;
    // If the related thread model instance is deadlocked, then the heartbeat reporting of the service will be stopped.
    if (!group_name.empty() && !CheckThreadModel(group_name)) {
      TRPC_FMT_ERROR(
          "ServiceHeartBeat service_name:{} instance:{} all his handles are dead. Stop reporting "
          "heartbeat to naming service:{}",
          service_name, group_name, registry_name_);
      continue;
    }
    report_function_(service_name, info_kv.second);
    ++report_count;
  }
}

void HeartBeatReport::DefaultReportFunction(const std::string& service_name,
                                            const ServiceHeartBeatInfo& heartbeat_info) {
  TrpcRegistryInfo registry_info;
  registry_info.plugin_name = registry_name_;
  RegistryInfo& reg_info = registry_info.registry_info;
  reg_info.name = service_name;
  reg_info.host = heartbeat_info.host;
  reg_info.port = heartbeat_info.port;

  if (!IsNeedReportServerHeartBeat()) {
    TRPC_FMT_DEBUG("server heartbeat report has been canceled.");
    return;
  }

  if (!IsNeedReportServiceHeartBeat(service_name)) {
    TRPC_FMT_DEBUG("service_name:{}, heartbeat report has been canceled.", service_name);
    return;
  }

  naming::AsyncHeartBeat(registry_info).Then([service_name, group_name = heartbeat_info.group_name](Future<>&& fut) {
    if (fut.IsFailed()) {
      TRPC_FMT_ERROR("Heartbeat failed, service:{}, group_name:{}, reason:{}, ", service_name, group_name,
                     fut.GetException().what());
    } else {
      TRPC_FMT_TRACE("Heartbeat success, service:{}, group_name:{}", service_name, group_name);
    }
    return MakeReadyFuture<>();
  });
}

uint32_t HeartBeatReport::CheckThreadHeartBeat() {
  uint32_t time_out_count = 0;
  std::unordered_set<std::string> incompetent_instance;
  for (auto const& instance_kv : thread_heartbeat_info_) {
    auto const& instance = instance_kv.first;
    bool incompetent = false;
    for (auto const& model_type_kv : instance_kv.second) {
      // If all thread in a certain role of the thread model instance are deadlocked, then the thread model instance is
      // considered deadlocked.
      if (CheckModelTypeIncompetent(model_type_kv.second, thread_time_out_ms_, &time_out_count)) {
        incompetent = true;
        TRPC_FMT_ERROR("ThreadHeartBeat {}, {}, all handles die", instance, GetThreadRoleString(model_type_kv.first));
      }
    }

    if (incompetent) {
      TRPC_FMT_ERROR("ThreadHeartBeat {} incompetent", instance);
      incompetent_instance.emplace(instance);
    }
  }
  incompetent_instances_.swap(incompetent_instance);
  return time_out_count;
}

void HeartBeatReport::SetServerHeartBeatReportSwitch(bool report_switch) {
  std::unique_lock<std::mutex> lock(report_switch_mutex_);
  report_switch_ = report_switch;
}

bool HeartBeatReport::IsNeedReportServerHeartBeat() {
  std::unique_lock<std::mutex> lock(report_switch_mutex_);
  return report_switch_;
}

void HeartBeatReport::SetServiceHeartBeatReportSwitch(const std::string& service_name, bool report_switch) {
  std::unique_lock<std::mutex> lock(report_switch_mutex_);
  service_heartbaet_switch_infos_[service_name] = report_switch;
}

bool HeartBeatReport::IsNeedReportServiceHeartBeat(const std::string& service_name) {
  std::unique_lock<std::mutex> lock(report_switch_mutex_);
  if (service_heartbaet_switch_infos_.find(service_name) != service_heartbaet_switch_infos_.end()) {
    return service_heartbaet_switch_infos_[service_name];
  }
  return true;
}

}  // namespace trpc
