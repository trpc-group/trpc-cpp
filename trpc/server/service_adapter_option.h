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

#include <string>
#include <vector>
#include <map>

#include "trpc/common/config/ssl_conf.h"

namespace trpc {

/// @brief Service adapter option
struct ServiceAdapterOption {
  /// Service name
  /// Format: trpc.app.server.pb service[.pb interface]
  /// The name of the service will be used as the name of the service discovery
  /// and registered on the naming service.
  std::string service_name;

  /// Socket type
  /// Two types: network(tcp/udp) socket/unix domain socket
  std::string socket_type;

  /// Service network type
  /// Three types: tcp/udp/tcp,udp
  std::string network;

  /// Service bind ip
  std::string ip;

  /// Whether the ip address is v6 format
  bool is_ipv6;

  /// Service bind port
  int port;

  /// Unix domain socket path
  std::string unix_path;

  /// Application layer protocol used by Service
  /// eg: trpc/http/...
  std::string protocol;

  /// Timeout(ms) for requests waiting in the recv queue to be executed
  uint32_t queue_timeout{60000};

  /// Timeout(ms) for idle connections
  uint32_t idle_time{60000};

  /// The maximum timeout(ms) for server request processing
  /// Unlimited by default
  uint32_t timeout{UINT32_MAX};

  /// Whether to ignore the timeout passed by the caller server
  bool disable_request_timeout{false};

  /// The maximum number of connections the Service allows to receive
  uint32_t max_conn_num{10000};

  /// The maximum size of the request packet that the Service allows to receive
  /// If set 0, disable check th packet size
  uint32_t max_packet_size{10000000};

  /// When the Service reads data from the network socket,
  /// the maximum data length allowed to be received at one time
  /// If set 0, not limited
  uint32_t recv_buffer_size{8192};

  /// When sending network data, the maximum data length of the io-send queue has cached
  /// Use in fiber runtime, if set 0, not limited
  uint32_t send_queue_capacity{0};

  /// When sending network data, the timeout(ms) of data in the io-send queue
  /// Use in fiber runtime
  uint32_t send_queue_timeout{3000};

  /// The number of threads(fibers) listening on the port
  uint32_t accept_thread_num{1};

  /// The thread model type use by service, deprecated.
  std::string threadmodel_type;

  /// The thread model instance name use by service
  /// If only use one thread model in global config, can not be set,
  /// framework will auto use the thread model in global config
  std::string threadmodel_instance_name;

  /// Under streaming, the timeout for reading messages from the stream
  int stream_read_timeout{32000};

  /// Under streaming, sliding window size(byte) for flow control in recv-side
  /// If set 0, disable flow control
  /// Currently only supports trpc streaming protocol
  uint32_t stream_max_window_size = 65535;

  /// SSL/TLS config
  ServerSslConfig ssl_config;

  /// The filter name use by this service
  std::vector<std::string> service_filters;

  /// Service filter config
  /// key is filter name
  std::map<std::string, std::any> service_filter_configs;
};

}  // namespace trpc
