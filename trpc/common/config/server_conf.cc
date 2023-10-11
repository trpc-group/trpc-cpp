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

#include "trpc/common/config/server_conf.h"

#include "trpc/util/log/logging.h"

namespace trpc {

void ServiceConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");

  TRPC_LOG_DEBUG("service_name:" << service_name);
  TRPC_LOG_DEBUG("socket_type:" << socket_type);
  TRPC_LOG_DEBUG("network:" << network);
  TRPC_LOG_DEBUG("ip:" << ip);
  TRPC_LOG_DEBUG("nic:" << nic);
  TRPC_LOG_DEBUG("is_ipv6:" << is_ipv6);
  TRPC_LOG_DEBUG("port:" << port);
  TRPC_LOG_DEBUG("unix_path:" << unix_path);
  TRPC_LOG_DEBUG("protocol:" << protocol);
  TRPC_LOG_DEBUG("max_conn_num:" << max_conn_num);
  TRPC_LOG_DEBUG("queue_timeout:" << queue_timeout);
  TRPC_LOG_DEBUG("idle_time:" << idle_time);
  TRPC_LOG_DEBUG("timeout:" << timeout);
  TRPC_LOG_DEBUG("disable_request_timeout:" << disable_request_timeout);
  TRPC_LOG_DEBUG("max_packet_size:" << max_packet_size);
  TRPC_LOG_DEBUG("recv_buffer_size:" << recv_buffer_size);
  TRPC_LOG_DEBUG("send_queue_capacity:" << send_queue_capacity);
  TRPC_LOG_DEBUG("send_queue_timeout:" << send_queue_timeout);
  TRPC_LOG_DEBUG("threadmodel_instance_name:" << threadmodel_instance_name);
  TRPC_LOG_DEBUG("accept_thread_num:" << accept_thread_num);
  TRPC_LOG_DEBUG("stream_read_timeout:" << stream_read_timeout);
  TRPC_LOG_DEBUG("stream_max_window_size:" << stream_max_window_size);

  ssl_config.Display();

  TRPC_LOG_DEBUG("--------------------------------");
}

void ServerConfig::Display() const {
  TRPC_LOG_DEBUG("==============server==============");
  TRPC_LOG_DEBUG("app:" << app);
  TRPC_LOG_DEBUG("server:" << server);
  TRPC_LOG_DEBUG("bin_path:" << bin_path);
  TRPC_LOG_DEBUG("conf_path:" << conf_path);
  TRPC_LOG_DEBUG("data_path:" << data_path);
  TRPC_LOG_DEBUG("admin_ip:" << admin_ip);
  TRPC_LOG_DEBUG("admin_port:" << admin_port);
  TRPC_LOG_DEBUG("admin_idle_time:" << admin_idle_time);
  TRPC_LOG_DEBUG("registry_name:" << registry_name);
  TRPC_LOG_DEBUG("enable_server_stats:" << enable_server_stats);
  TRPC_LOG_DEBUG("server_stats_interval:" << server_stats_interval);

  for (const auto& i : services_config) {
    i.Display();
  }

  TRPC_LOG_DEBUG("==============server==============");
}

}  // namespace trpc
