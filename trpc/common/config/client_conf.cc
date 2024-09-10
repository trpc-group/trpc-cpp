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

#include "trpc/common/config/client_conf.h"

#include "trpc/util/log/logging.h"

namespace trpc {

void ClientConfig::Display() const {
  TRPC_LOG_DEBUG("-------------client-------------");

  for (const auto& it : service_proxy_config) {
    it.Display();
  }

  TRPC_LOG_DEBUG("-------------client-------------");
}

void ServiceProxyConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");

  TRPC_LOG_DEBUG("name:" << name);
  TRPC_LOG_DEBUG("target:" << target);
  TRPC_LOG_DEBUG("protocol:" << protocol);
  TRPC_LOG_DEBUG("network:" << network);
  TRPC_LOG_DEBUG("conn_type:" << conn_type);
  TRPC_LOG_DEBUG("timeout:" << timeout);
  TRPC_LOG_DEBUG("selector_name:" << selector_name);
  TRPC_LOG_DEBUG("callee_set_name:" << callee_set_name);
  TRPC_LOG_DEBUG("load_balance_name:" << load_balance_name);
  TRPC_LOG_DEBUG("load_balance_type:" << load_balance_type);
  TRPC_LOG_DEBUG("disable_servicerouter:" << disable_servicerouter);
  TRPC_LOG_DEBUG("is_conn_complex:" << is_conn_complex);
  TRPC_LOG_DEBUG("max_packet_size:" << max_packet_size);
  TRPC_LOG_DEBUG("recv_buffer_size:" << recv_buffer_size);
  TRPC_LOG_DEBUG("send_queue_capacity:" << send_queue_capacity);
  TRPC_LOG_DEBUG("send_queue_timeout:" << send_queue_timeout);
  TRPC_LOG_DEBUG("max_conn_num:" << max_conn_num);
  TRPC_LOG_DEBUG("request_timeout_check_interval:" << request_timeout_check_interval);
  TRPC_LOG_DEBUG("is_reconnection:" << is_reconnection);
  TRPC_LOG_DEBUG("connect_timeout:" << connect_timeout);
  TRPC_LOG_DEBUG("allow_reconnect:" << allow_reconnect);
  TRPC_LOG_DEBUG("idle_time:" << idle_time);
  TRPC_LOG_DEBUG("threadmodel_instance_name:" << threadmodel_instance_name);
  TRPC_LOG_DEBUG("support_pipeline:" << support_pipeline);

  if (redis_conf.enable) {
    redis_conf.Display();
  }

  if (mysql_conf.enable) {
    mysql_conf.Display();
  }

  // SSL/TLS config
  ssl_config.Display();

  if (!service_filter_configs.empty()) {
    auto iter = service_filter_configs.find(kRetryHedgingLimitFilter);
    if (iter != service_filter_configs.end()) {
      std::any_cast<RetryHedgingLimitConfig>(iter->second).Display();
    }
  }

  TRPC_LOG_DEBUG("--------------------------------");
}

}  // namespace trpc
