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

#include "trpc/client/service_proxy_manager.h"

#include "trpc/util/net_util.h"

namespace trpc {

void ServiceProxyManager::SetOptionFromConfig(const ServiceProxyConfig& proxy_conf,
                                              std::shared_ptr<ServiceProxyOption>& option) {
  option->codec_name = proxy_conf.protocol;
  option->network = proxy_conf.network;
  option->conn_type = proxy_conf.conn_type;
  option->timeout = proxy_conf.timeout;
  option->selector_name = proxy_conf.selector_name;
  option->name_space = proxy_conf.namespace_;
  option->target = proxy_conf.target;
  option->callee_name = proxy_conf.callee_name;
  option->callee_set_name = proxy_conf.callee_set_name;
  option->load_balance_name = proxy_conf.load_balance_name;
  option->disable_servicerouter = proxy_conf.disable_servicerouter;
  option->is_conn_complex = proxy_conf.is_conn_complex;
  option->max_packet_size = proxy_conf.max_packet_size;
  option->recv_buffer_size = proxy_conf.recv_buffer_size;
  option->send_queue_capacity = proxy_conf.send_queue_capacity;
  option->send_queue_timeout = proxy_conf.send_queue_timeout;
  option->max_conn_num = proxy_conf.max_conn_num;
  option->idle_time = proxy_conf.idle_time;
  option->request_timeout_check_interval = proxy_conf.request_timeout_check_interval;
  option->is_reconnection = proxy_conf.is_reconnection;
  option->connect_timeout = proxy_conf.connect_timeout;
  option->allow_reconnect = proxy_conf.allow_reconnect;
  option->threadmodel_type_name = proxy_conf.threadmodel_type;
  option->threadmodel_instance_name = proxy_conf.threadmodel_instance_name;
  option->service_filters = proxy_conf.service_filters;

  option->load_balance_type = GetLoadBalanceType(proxy_conf.load_balance_type);

  option->redis_conf = proxy_conf.redis_conf;
  // Set SSL/TLS config for client.
  option->ssl_config = proxy_conf.ssl_config;
  option->support_pipeline = proxy_conf.support_pipeline;
  option->fiber_pipeline_connector_queue_size = proxy_conf.fiber_pipeline_connector_queue_size;
  option->fiber_connpool_shards = proxy_conf.fiber_connpool_shards;

  option->service_filter_configs = proxy_conf.service_filter_configs;

  option->stream_max_window_size = proxy_conf.stream_max_window_size;
}

void ServiceProxyManager::SetOptionDefaultValue(const std::string& name, std::shared_ptr<ServiceProxyOption>& option) {
  // The name parameter of option is consistent with the name parameter of GetProxy.
  option->name = name;

  // If name_space is not set, it will be set to the name_space configured in global configuration.
  if (option->name_space.empty()) {
    option->name_space = TrpcConfig::GetInstance()->GetGlobalConfig().env_namespace;
  }
  
  // If callee_name is not set, set it to name of proxy.
  if (option->callee_name.empty()) {
    option->callee_name = name;
  }

  // If target is not set, set it to name of proxy.
  if (option->target.empty()) {
    option->target = name;
  }

  // If the target is "ip:port" format, set the selector_name to direct automatically.
  if (util::IsValidIpPorts(option->target) && option->selector_name != "direct") {
    option->selector_name = "direct";
  }

  // If caller_name is not set, set it to trpc.{app}.{server}.
  if (option->caller_name.empty()) {
    option->caller_name = "trpc." + TrpcConfig::GetInstance()->GetServerConfig().app + "." +
                          TrpcConfig::GetInstance()->GetServerConfig().server;
  }
}

void ServiceProxyManager::Stop() {
  for (int i = 0; i != kShards; ++i) {
    auto& shard = shards_[i];
    std::scoped_lock _(shard.lock);

    for (auto& it : shard.service_proxys) {
      it.second->Stop();
    }
  }
}

void ServiceProxyManager::Destroy() {
  for (int i = 0; i < kShards; ++i) {
    auto& shard = shards_[i];
    std::scoped_lock _(shard.lock);

    for (auto& it : shard.service_proxys) {
      it.second->Destroy();
    }

    shard.service_proxys.clear();
  }
}

}  // namespace trpc
