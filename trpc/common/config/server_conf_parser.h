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

#include "yaml-cpp/yaml.h"

#include "trpc/common/config/server_conf.h"
#include "trpc/common/config/ssl_conf_parser.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/net_util.h"

namespace YAML {

template <>
struct convert<trpc::ServiceConfig> {
  static YAML::Node encode(const trpc::ServiceConfig& service_config) {
    YAML::Node node;

    node["name"] = service_config.service_name;
    node["socket_type"] = service_config.socket_type;
    node["network"] = service_config.network;
    if (!service_config.ip.empty()) {
      node["ip"] = service_config.ip;
    }
    if (!service_config.nic.empty()) {
      node["nic"] = service_config.nic;
    }
    node["port"] = service_config.port;
    node["unix_path"] = service_config.unix_path;
    node["protocol"] = service_config.protocol;
    node["max_conn_num"] = service_config.max_conn_num;
    node["queue_timeout"] = service_config.queue_timeout;
    node["idle_time"] = service_config.idle_time;
    node["timeout"] = service_config.timeout;
    node["disable_request_timeout"] = service_config.disable_request_timeout;
    node["share_transport"] = service_config.share_transport;
    node["max_packet_size"] = service_config.max_packet_size;
    node["recv_buffer_size"] = service_config.recv_buffer_size;
    node["send_queue_capacity"] = service_config.send_queue_capacity;
    node["send_queue_timeout"] = service_config.send_queue_timeout;
    node["threadmodel_type"] = service_config.threadmodel_type;
    node["threadmodel_instance_name"] = service_config.threadmodel_instance_name;
    node["accept_thread_num"] = service_config.accept_thread_num;
    node["stream_read_timeout"] = service_config.stream_read_timeout;
    node["stream_max_window_size"] = service_config.stream_max_window_size;
    node["filter"] = service_config.service_filters;
    node["ssl"] = service_config.ssl_config;

    return node;
  }

  static void decode_ip(const YAML::Node& node, trpc::ServiceConfig& service_config) {
    if (node["nic"]) {
      service_config.nic = node["nic"].as<std::string>();
    }
    if (node["ip"]) {
      service_config.ip = node["ip"].as<std::string>();
    } else if (!service_config.nic.empty()) {
      service_config.ip = trpc::util::GetIpByEth(service_config.nic);
    }

    service_config.is_ipv6 = (service_config.ip.find(':') != std::string::npos);
  }

  static bool decode(const YAML::Node& node, trpc::ServiceConfig& service_config) {
    service_config.service_name = node["name"].as<std::string>();
    if (node["socket_type"]) {
      service_config.socket_type = node["socket_type"].as<std::string>();
    }
    if (node["network"]) {
      service_config.network = node["network"].as<std::string>();
    }
    decode_ip(node, service_config);

    if (node["port"]) {
      service_config.port = node["port"].as<int>();
    }
    if (node["unix_path"]) {
      service_config.unix_path = node["unix_path"].as<std::string>();
    }
    service_config.protocol = node["protocol"].as<std::string>();
    if (service_config.protocol == "http") {
      service_config.max_packet_size = 0;
    }
    if (node["max_conn_num"]) {
      service_config.max_conn_num = node["max_conn_num"].as<uint32_t>();
    }
    if (node["queue_timeout"]) {
      service_config.queue_timeout = node["queue_timeout"].as<uint32_t>();
    }
    if (node["idle_time"]) {
      service_config.idle_time = node["idle_time"].as<uint32_t>();
    }
    if (node["timeout"]) {
      service_config.timeout = node["timeout"].as<uint32_t>();
    }
    if (node["disable_request_timeout"]) {
      service_config.disable_request_timeout = node["disable_request_timeout"].as<bool>();
    }
    if (node["share_transport"]) {
      service_config.share_transport = node["share_transport"].as<bool>();
    }
    if (node["max_packet_size"]) {
      service_config.max_packet_size = node["max_packet_size"].as<uint32_t>();
    }
    if (node["recv_buffer_size"]) {
      service_config.recv_buffer_size = node["recv_buffer_size"].as<uint32_t>();
    }
    if (node["send_queue_capacity"]) {
      service_config.send_queue_capacity = node["send_queue_capacity"].as<uint32_t>();
    }
    if (node["send_queue_timeout"]) {
      service_config.send_queue_timeout = node["send_queue_timeout"].as<uint32_t>();
    }
    if (node["threadmodel_type"]) {
      service_config.threadmodel_type = node["threadmodel_type"].as<std::string>();
    }
    if (node["threadmodel_instance_name"]) {
      service_config.threadmodel_instance_name = node["threadmodel_instance_name"].as<std::string>();
    }

    // stream read timeout
    if (node["stream_read_timeout"]) {
      service_config.stream_read_timeout = node["stream_read_timeout"].as<int>();
    }

    if (node["stream_max_window_size"]) {
      service_config.stream_max_window_size = node["stream_max_window_size"].as<uint32_t>();
    }

    if (node["accept_thread_num"]) {
      service_config.accept_thread_num = node["accept_thread_num"].as<uint32_t>();
#if !defined(SO_REUSEPORT) || defined(TRPC_DISABLE_REUSEPORT)
      if (service_config.accept_thread_num > 1) {
        TRPC_LOG_WARN(
            "reuseport is not supported, set accpet_thread_num=1 as default, please recomplie with [--define "
            "trpc_disable_reuseport=false] and linux kernel >= 3.9");
        service_config.accept_thread_num = 1;
      }
#endif
    }
    if (node["filter"]) {
      service_config.service_filters = node["filter"].as<std::vector<std::string>>();
    }

    if (node["ssl"]) {
      service_config.ssl_config = node["ssl"].as<trpc::ServerSslConfig>();
    }

    return true;
  }
};

template <>
struct convert<trpc::ServerConfig> {
  static YAML::Node encode(const trpc::ServerConfig& server_config) {
    YAML::Node node;

    node["app"] = server_config.app;
    node["server"] = server_config.server;
    node["bin_path"] = server_config.bin_path;
    node["conf_path"] = server_config.conf_path;
    node["data_path"] = server_config.data_path;
    node["admin_ip"] = server_config.admin_ip;
    node["admin_port"] = server_config.admin_port;
    node["admin_idle_time"] = server_config.admin_idle_time;
    node["registry_name"] = server_config.registry_name;
    node["enable_server_stats"] = server_config.enable_server_stats;
    node["server_stats_interval"] = server_config.server_stats_interval;
    node["filter"] = server_config.filters;
    node["service"] = server_config.services_config;
    node["stop_max_wait_time"] = server_config.stop_max_wait_time;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::ServerConfig& server_config) {
    server_config.app = node["app"].as<std::string>();
    server_config.server = node["server"].as<std::string>();

    if (node["bin_path"]) {
      server_config.bin_path = node["bin_path"].as<std::string>();
    }

    if (node["conf_path"]) {
      server_config.conf_path = node["conf_path"].as<std::string>();
    }

    if (node["data_path"]) {
      server_config.data_path = node["data_path"].as<std::string>();
    }

    if (node["admin_ip"]) {
      server_config.admin_ip = node["admin_ip"].as<std::string>();
    }

    if (node["admin_port"]) {
      server_config.admin_port = node["admin_port"].as<std::string>();
    }

    if (node["admin_idle_time"]) {
      server_config.admin_idle_time = node["admin_idle_time"].as<uint32_t>();
    }

    if (node["registry_name"]) {
      server_config.registry_name = node["registry_name"].as<std::string>();
    }

    if (node["enable_server_stats"]) {
      server_config.enable_server_stats = node["enable_server_stats"].as<bool>();
    }

    if (node["server_stats_interval"]) {
      server_config.server_stats_interval = node["server_stats_interval"].as<uint32_t>();
    }

    if (node["filter"]) {
      for (size_t idx = 0; idx < node["filter"].size(); ++idx) {
        server_config.filters.push_back(node["filter"][idx].as<std::string>());
      }
    }

    if (node["service"]) {
      for (size_t idx = 0; idx < node["service"].size(); ++idx) {
        auto item = node["service"][idx].as<trpc::ServiceConfig>();
        if (item.accept_thread_num == 0) {
          item.accept_thread_num = 1;
        }
        server_config.services_config.push_back(item);
      }
    }

    if (node["stop_max_wait_time"]) {
      server_config.stop_max_wait_time = node["stop_max_wait_time"].as<uint32_t>();
    }

    return true;
  }
};

}  // namespace YAML
