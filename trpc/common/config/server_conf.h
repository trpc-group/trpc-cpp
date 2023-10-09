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

#include <any>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "yaml-cpp/yaml.h"

#include "trpc/common/config/ssl_conf.h"

namespace trpc {

/// @brief Service Configuration
struct ServiceConfig {
  /// @brief Service name
  /// Format: trpc.app.server.pb service[.pb interface]
  /// The name of the service will be used as the name of the service discovery
  /// and registered on the naming service.
  std::string service_name;

  /// @brief Socket type
  /// Two types: network(tcp/udp) socket/unix domain socket
  std::string socket_type{"net"};

  /// @brief Service network type
  /// Three types: tcp/udp/tcp,udp
  std::string network{"tcp"};

  /// @brief Service bind ip
  std::string ip{"0.0.0.0"};

  /// @brief Nic name
  /// Use `ip` first, if `ip` is empty, use it
  std::string nic;

  /// @brief Service bind port
  int port{-1};

  /// @brief Whether the ip address is v6 format
  bool is_ipv6{false};

  /// @brief Unix domain socket path
  std::string unix_path;

  /// @brief Application layer protocol used by Service
  /// eg: trpc/http/...
  std::string protocol{"trpc"};

  /// @brief The maximum number of connections the Service allows to receive
  uint32_t max_conn_num{10000};

  /// @brief Timeout(ms) for requests waiting in the recv queue to be executed
  uint32_t queue_timeout{5000};

  /// @brief Timeout(ms) for idle connections
  uint32_t idle_time{60000};

  /// @brief The maximum timeout(ms) for server request processing
  /// Unlimited by default
  uint32_t timeout{UINT32_MAX};

  /// @brief Whether to ignore the timeout passed by the caller server
  bool disable_request_timeout{false};

  /// @brief Whether the service will share the same ip/port
  /// If multi-service‘s `ip/port/protocol/..` config is same, framework will auto share
  bool share_transport = true;

  /// @brief The maximum size of the request packet that the Service allows to receive
  /// If set 0, disable check th packet size
  uint32_t max_packet_size{10000000};

  /// @brief When the Service reads data from the network socket,
  /// the maximum data length allowed to be received at one time
  /// If set 0, not limited
  uint32_t recv_buffer_size{10000000};

  /// @brief When sending network data, the maximum data length of the io-send queue has cached
  /// Use in fiber runtime, if set 0, not limited
  uint32_t send_queue_capacity{0};

  /// @brief When sending network data, the timeout(ms) of data in the io-send queue
  /// Use in fiber runtime
  uint32_t send_queue_timeout{3000};

  /// @brief The thread model type use by service, deprecated.
  std::string threadmodel_type;

  /// @brief The thread model instance name use by service
  /// If only use one thread model in global config, can not be set,
  /// framework will auto use the thread model in global config
  std::string threadmodel_instance_name;

  /// @brief The number of threads(fibers) listening on the port
  uint32_t accept_thread_num{1};

  /// @brief Under streaming, the timeout for reading messages from the stream
  int stream_read_timeout{3000};

  /// @brief Under streaming, sliding window size(byte) for flow control in recv-side
  /// If set 0, disable flow control
  /// Currently only supports trpc streaming protocol
  uint32_t stream_max_window_size = 65535;

  /// @brief SSL/TLS config
  ServerSslConfig ssl_config;

  /// @brief The filter name use by this service
  std::vector<std::string> service_filters;

  /// @brief Service filter config
  /// key is filter name
  std::map<std::string, std::any> service_filter_configs;

  void Display() const;
};

/// @brief Server config
struct ServerConfig {
  /// @brief Business appplication name
  std::string app;

  /// @brief Business server name
  std::string server;

  /// @brief The path where the executable program file of the server is located
  std::string bin_path{"/usr/local/trpc/bin/"};

  /// @brief The path where the framework configuration file of the service is located
  std::string conf_path{"/usr/local/trpc/conf/"};

  /// @brief The path where the business data file of the service is located
  std::string data_path{"/usr/local/trpc/data/"};

  /// @brief The ip addr of admin service
  std::string admin_ip;

  /// @brief The port of admin service
  std::string admin_port;

  /// @brief The idle connection timeout(ms) of admin service
  uint32_t admin_idle_time{60000};

  /// @brief The name of naming registry
  std::string registry_name;

  /// @brief Whether to enable service self-registration
  bool enable_self_register{false};

  /// @brief Whether to enable server stats(qps/avg-time/...)
  bool enable_server_stats{false};

  /// @brief Server stats interval, default 1min
  uint32_t server_stats_interval{60000};

  /// @brief Service configuration collection
  std::vector<ServiceConfig> services_config;

  /// @brief Filters used by all services
  std::vector<std::string> filters;

  void Display() const;
};

}  // namespace trpc
