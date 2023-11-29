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

#include "trpc/server/trpc_server.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include "trpc/common/config/trpc_config.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/runtime/common/heartbeat/heartbeat_report.h"
#include "trpc/runtime/common/stats/frame_stats.h"
#include "trpc/runtime/runtime.h"
#include "trpc/runtime/threadmodel/thread_model.h"
#include "trpc/util/chrono/time.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/string/string_util.h"

using namespace std::literals;

namespace trpc {

TrpcServer::TrpcServer(const ServerConfig& server_config) : terminate_(false), server_config_(server_config) {}

bool TrpcServer::Initialize() {
  if (state_ != ServerState::kInitialized) {
    BuildAdminServiceAdapter();

    BuildBusinessServerAdapter();

    if (TrpcConfig::GetInstance()->GetGlobalConfig().heartbeat_config.enable_heartbeat) {
      HeartBeatReport::GetInstance()->Start();
    }

    state_ = ServerState::kInitialized;
  }

  return true;
}

void TrpcServer::BuildServiceAdapterOption(const ServiceConfig& config, ServiceAdapterOption& option) {
  option.service_name = config.service_name;
  option.socket_type = config.socket_type;
  option.network = config.network;
  option.ip = config.ip;
  option.is_ipv6 = (option.ip.find(':') != std::string::npos);
  option.port = config.port;
  option.unix_path = config.unix_path;
  option.protocol = config.protocol;
  option.queue_timeout = config.queue_timeout;
  option.idle_time = config.idle_time;
  option.timeout = config.timeout;
  option.disable_request_timeout = config.disable_request_timeout;
  option.max_conn_num = config.max_conn_num;
  option.max_packet_size = config.max_packet_size;
  option.recv_buffer_size = config.recv_buffer_size;
  option.send_queue_capacity = config.send_queue_capacity;
  option.send_queue_timeout = config.send_queue_timeout;
  option.accept_thread_num = config.accept_thread_num;
  option.threadmodel_type = config.threadmodel_type;
  option.threadmodel_instance_name = config.threadmodel_instance_name;
  option.stream_read_timeout = config.stream_read_timeout;
  option.stream_max_window_size = config.stream_max_window_size;
  option.ssl_config = config.ssl_config;
  option.service_filters = config.service_filters;
  option.service_filter_configs = config.service_filter_configs;
  // Stream read timeout and window size.
  option.stream_read_timeout = config.stream_read_timeout;
  option.stream_max_window_size = config.stream_max_window_size;
}

void TrpcServer::BuildAdminServiceAdapter() {
  int port = 0;
  try {
    port = std::atoi(server_config_.admin_port.c_str());
  } catch (std::exception& ex) {
    TRPC_FMT_ERROR("admin service port `{}` error.");
    return;
  }

  if (!server_config_.admin_ip.empty() && port > 0) {
    ServiceAdapterOption option;
    option.service_name = "trpc." + server_config_.app + "." + server_config_.server + ".AdminService";
    option.network = "tcp";
    option.ip = server_config_.admin_ip;
    option.is_ipv6 = false;
    option.port = atoi(server_config_.admin_port.c_str());
    option.protocol = "http";
    option.queue_timeout = 60000;
    option.idle_time = server_config_.admin_idle_time;
    option.timeout = UINT32_MAX;
    option.disable_request_timeout = false;
    option.max_conn_num = 100;
    option.max_packet_size = UINT32_MAX;
    option.threadmodel_instance_name = std::string(kSeparateAdminInstance);

    auto service_adapter = std::make_shared<ServiceAdapter>(std::move(option));

    admin_service_name_ = option.service_name;
    admin_service_ = std::make_shared<trpc::AdminService>();
    admin_service_->SetAdapter(service_adapter.get());

    service_adapter->SetService(admin_service_);
    service_adapter->SetAutoStart();

    service_adapters_[admin_service_name_] = service_adapter;
  }
}

void TrpcServer::BuildBusinessServerAdapter() {
  std::unordered_map<std::string, ServiceConfig> share_transport_service_conf;
  for (const auto& config : server_config_.services_config) {
    //  ----------
    // | service1 |
    // | service2 |====>service_adapter====>transport
    // | serviceN |
    //  ---------
    auto share_name = GetSharedKey(config);
    auto adapter_it = service_shared_adapters_.find(share_name);
    if (adapter_it != service_shared_adapters_.end()) {
      TRPC_ASSERT(CheckSharedTransportConfig(share_transport_service_conf[share_name], config) &&
                  "Please Check Service Config");
      ServiceAdapterPtr adapter = adapter_it->second;
      adapter->SetIsShared(true);

      service_adapters_[config.service_name] = adapter;
    } else {
      service_shared_adapters_[share_name] = BuildServiceAdapter(config);
      share_transport_service_conf[share_name] = config;
    }
  }
}

ServiceAdapterPtr TrpcServer::BuildServiceAdapter(const ServiceConfig& config) {
  if (service_adapters_.find(config.service_name) != service_adapters_.end()) {
    TRPC_FMT_ERROR("service {} already exist.", config.service_name);
    return nullptr;
  }

  ServiceAdapterOption option;
  BuildServiceAdapterOption(config, option);

  ServiceAdapterPtr service_adapter(new ServiceAdapter(std::move(option)));
  service_adapters_[config.service_name] = service_adapter;

  return service_adapter;
}

bool TrpcServer::Start() {
  for (const auto& iter : service_adapters_) {
    if (iter.second->IsAutoStart()) {
      TRPC_FMT_INFO("Service {} auto-start to listen ...", iter.first);
      if (!iter.second->Listen()) {
        return false;
      }
    } else {
      TRPC_FMT_DEBUG("Service {} is not auto-started.", iter.first);
    }
  }

  state_ = ServerState::kStart;

  return true;
}

void TrpcServer::WaitForShutdown() {
  while (!terminate_) {
    if (runtime::IsInFiberRuntime()) {
      // fiber sleep
      FiberSleepFor(1s);
    } else {
      // default thread sleep
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (terminate_function_) {
      bool ret = terminate_function_();
      if (ret) {
        TRPC_LOG_ERROR("terminate signal.");
        terminate_ = true;
      }
    }
  }

  Stop();

  state_ = ServerState::kShutDown;
}

void TrpcServer::Stop() {
  if (TrpcConfig::GetInstance()->GetGlobalConfig().heartbeat_config.enable_heartbeat) {
    HeartBeatReport::GetInstance()->Stop();
  }

  for (const auto& iter : service_adapters_) {
    if (!server_config_.registry_name.empty()) {
      TRPC_LOG_DEBUG(iter.first << " start to UnregisterName...");
      UnregisterName(iter.second);
    }
  }

  uint64_t begin_stop_time = trpc::time::GetMilliSeconds();
  while (FrameStats::GetInstance()->GetServerStats().GetReqConcurrency() > 0) {
    TRPC_LOG_DEBUG("current ReqConcurrency:" << FrameStats::GetInstance()->GetServerStats().GetReqConcurrency()
                                             << ",mso need wait until request be all handled over");
    if (runtime::IsInFiberRuntime()) {
      FiberSleepFor(std::chrono::milliseconds(100));
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // check if wait more than max_wait_time，just break and continue stop
    if (trpc::time::GetMilliSeconds() - begin_stop_time > server_config_.stop_max_wait_time) {
      break;
    }
  }

  TRPC_LOG_DEBUG(" and current ReqConcurrency:" << FrameStats::GetInstance()->GetServerStats().GetReqConcurrency());

  std::vector<ServiceAdapterPtr> stoped_adapters;
  for (const auto& iter : service_adapters_) {
    TRPC_LOG_DEBUG(iter.first << " start to stop...");
    auto stoped_adapter_it = std::find(stoped_adapters.begin(), stoped_adapters.end(), iter.second);
    if (stoped_adapter_it != stoped_adapters.end()) {
      continue;
    }

    iter.second->Stop();
    stoped_adapters.push_back(iter.second);
  }
}

void TrpcServer::Destroy() {
  std::vector<ServiceAdapterPtr> destroyed_adapters;
  for (const auto& iter : service_adapters_) {
    TRPC_LOG_INFO(iter.first << " start to destroy...");
    auto destroyed_adapter_it = std::find(destroyed_adapters.begin(), destroyed_adapters.end(), iter.second);
    if (destroyed_adapter_it != destroyed_adapters.end()) {
      continue;
    }

    iter.second->Destroy();
    destroyed_adapters.push_back(iter.second);
  }

  service_adapters_.clear();
  service_shared_adapters_.clear();

  state_ = ServerState::kDestroy;
}

TrpcServer::RegisterRetCode TrpcServer::RegisterService(const std::string& service_name, ServicePtr& service,
                                                        bool auto_start) {
  if (service_name.empty()) {
    TRPC_FMT_ERROR("service_name is empty");
    return RegisterRetCode::kParameterError;
  }

  if (service == nullptr) {
    TRPC_FMT_ERROR("service_name:{} service is nullptr", service_name);
    return RegisterRetCode::kParameterError;
  }

  auto service_adapter_it = service_adapters_.find(service_name);
  if (service_adapter_it == service_adapters_.end()) {
    TRPC_FMT_ERROR("service_name:{} not found in server config", service_name);
    return RegisterRetCode::kSearchError;
  }
  service->SetName(service_name);
  service_adapter_it->second->SetService(service);
  service->SetAdapter(service_adapter_it->second.get());

  if (auto_start) {
    service_adapter_it->second->SetAutoStart();
  }

  if (!server_config_.registry_name.empty()) {
    if (RegisterName(service_adapter_it->second) != 0) {
      TRPC_FMT_ERROR("service_name:{} self register service failed", service_name);
      return RegisterRetCode::kRegisterError;
    }
  }

  if (TrpcConfig::GetInstance()->GetGlobalConfig().heartbeat_config.enable_heartbeat) {
    TRPC_FMT_DEBUG("service_name:{} report heartbeat info", service_name);

    ServiceHeartBeatInfo heartbeat_info;
    auto const& option = service_adapter_it->second->GetServiceAdapterOption();
    heartbeat_info.service_name = service_name;
    heartbeat_info.host = option.ip;
    heartbeat_info.port = option.port;
    heartbeat_info.group_name = option.threadmodel_instance_name;
    HeartBeatReport::GetInstance()->RegisterServiceHeartBeatInfo(std::move(heartbeat_info));
  }

  return RegisterRetCode::kOk;
}

TrpcServer::RegisterRetCode TrpcServer::RegisterService(const ServiceConfig& config, ServicePtr& service,
                                                        bool auto_start) {
  auto share_name = GetSharedKey(config);
  if (service_shared_adapters_.find(share_name) != service_shared_adapters_.end()) {
    TRPC_FMT_ERROR("It is not allowed to change multi-service {} dynamically", config.service_name);
    return RegisterRetCode::kParameterError;
  }

  auto service_adapter = BuildServiceAdapter(config);
  if (!service_adapter) {
    return RegisterRetCode::kParameterError;
  }

  RegisterRetCode ret = RegisterService(config.service_name, service, auto_start);
  if (ret != RegisterRetCode::kOk) {
    return ret;
  }

  TRPC_FMT_INFO("Service {} start to listen ...", config.service_name);

  if (auto_start) {
    if (!service_adapter->Listen()) {
      return RegisterRetCode::kUnknownError;
    }
  }

  return RegisterRetCode::kOk;
}

bool TrpcServer::StartService(const std::string& service_name) {
  auto service_adapter_it = service_adapters_.find(service_name);
  if (service_adapter_it == service_adapters_.end()) {
    TRPC_FMT_ERROR("service_name:{} doesn't exist.", service_name);
    return false;
  }

  if (service_adapter_it->second->IsAutoStart()) {
    TRPC_FMT_ERROR("Service {} is auto-started.", service_name);
    return false;
  }

  service_adapter_it->second->SetAutoStart();

  TRPC_FMT_INFO("Service {} start to listen ...", service_name);

  if (!service_adapter_it->second->Listen()) {
    return false;
  }

  return true;
}

bool TrpcServer::StartService(const ServiceConfig& config, ServicePtr& service) {
  return RegisterService(config, service, true) == RegisterRetCode::kOk;
}

bool TrpcServer::StopService(const std::string& service_name, bool clean_conn) {
  auto service_adapter_it = service_adapters_.find(service_name);
  if (service_adapter_it == service_adapters_.end()) {
    TRPC_FMT_ERROR("service_name:{} doesn't exist.", service_name);
    return false;
  }

  TRPC_FMT_INFO("Service {} stop to listen ...", service_name);

  return service_adapter_it->second->StopListen(clean_conn);
}

int TrpcServer::RegisterName(const ServiceAdapterPtr& service_adapter) {
  const std::string& service_name = service_adapter->GetServiceName();
  if (service_name == admin_service_name_) {
    return 0;
  }

  TRPC_LOG_DEBUG("service_name:" << service_name << " self register service.");
  auto const& option = service_adapter->GetServiceAdapterOption();
  TrpcRegistryInfo registry_info;
  BuildTrpcRegistryInfo(option, registry_info);
  if (naming::Register(registry_info) == 0) {
    return 0;
  }

  TRPC_LOG_ERROR("service_name:" << service_name << " register to plugin name: " << server_config_.registry_name
                                 << " failed.");
  return -1;
}

int TrpcServer::UnregisterName(const ServiceAdapterPtr& service_adapter) {
  const std::string& service_name = service_adapter->GetServiceName();
  if (service_name == admin_service_name_) {
    return 0;
  }

  TRPC_LOG_DEBUG("service_name:" << service_name << " self unregister service.");
  auto const& option = service_adapter->GetServiceAdapterOption();
  TrpcRegistryInfo registry_info;
  BuildTrpcRegistryInfo(option, registry_info);
  if (naming::Unregister(registry_info) == 0) {
    return 0;
  }

  TRPC_LOG_ERROR("service_name:" << service_name << " unregister to plugin name: " << server_config_.registry_name
                                 << " failed.");
  return -1;
}

void TrpcServer::BuildTrpcRegistryInfo(const ServiceAdapterOption& option, TrpcRegistryInfo& registry_info) {
  registry_info.plugin_name = server_config_.registry_name;
  registry_info.registry_info.name = option.service_name;
  registry_info.registry_info.host = option.ip;
  registry_info.registry_info.port = option.port;
}

std::shared_ptr<trpc::AdminService> TrpcServer::GetAdminService() { return admin_service_; }

bool TrpcServer::CheckSharedTransportConfig(const ServiceConfig& first_service_conf,
                                            const ServiceConfig& service_conf) {
  bool is_compare = first_service_conf.max_conn_num == service_conf.max_conn_num;
  is_compare &= (first_service_conf.socket_type == service_conf.socket_type);
  is_compare &= (first_service_conf.queue_timeout == service_conf.queue_timeout);
  is_compare &= (first_service_conf.max_packet_size == service_conf.max_packet_size);
  is_compare &= (first_service_conf.threadmodel_instance_name == service_conf.threadmodel_instance_name);

  return is_compare;
}

std::string TrpcServer::GetSharedKey(const ServiceConfig& config) {
  if (config.port == -1) {
    return config.service_name;
  }
  return trpc::util::FormatString("{}_{}_{}_{}", config.ip, config.port, config.network, config.protocol);
}

void TrpcServer::SetServerHeartBeatReportSwitch(bool report_switch) {
  HeartBeatReport::GetInstance()->SetServerHeartBeatReportSwitch(report_switch);
}

bool TrpcServer::GetServerHeartBeatReportSwitch() {
  return HeartBeatReport::GetInstance()->IsNeedReportServerHeartBeat();
}

std::shared_ptr<TrpcServer> GetTrpcServer() {
  static std::shared_ptr<TrpcServer> server =
      std::make_shared<TrpcServer>(TrpcConfig::GetInstance()->GetServerConfig());
  return server;
}

}  // namespace trpc
