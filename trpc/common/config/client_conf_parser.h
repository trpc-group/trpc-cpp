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

#include "trpc/common/config/client_conf.h"
#include "trpc/common/config/redis_client_conf_parser.h"
#include "trpc/common/config/retry_conf_parser.h"
#include "trpc/common/config/ssl_conf_parser.h"

namespace YAML {

template <>
struct convert<trpc::ServiceProxyConfig> {
  static YAML::Node encode(const trpc::ServiceProxyConfig& proxy_config) {
    YAML::Node node;

    node["name"] = proxy_config.name;
    node["target"] = proxy_config.target;
    node["protocol"] = proxy_config.protocol;
    node["network"] = proxy_config.network;
    node["conn_type"] = proxy_config.conn_type;
    node["is_conn_complex"] = proxy_config.is_conn_complex;
    node["support_pipeline"] = proxy_config.support_pipeline;
    node["fiber_pipeline_connector_queue_size"] = proxy_config.fiber_pipeline_connector_queue_size;
    node["connect_timeout"] = proxy_config.connect_timeout;
    node["timeout"] = proxy_config.timeout;
    node["request_timeout_check_interval"] = proxy_config.request_timeout_check_interval;
    node["is_reconnection"] = proxy_config.is_reconnection;
    node["allow_reconnect"] = proxy_config.allow_reconnect;
    node["max_packet_size"] = proxy_config.max_packet_size;
    node["max_conn_num"] = proxy_config.max_conn_num;
    node["idle_time"] = proxy_config.idle_time;
    node["recv_buffer_size"] = proxy_config.recv_buffer_size;
    node["send_queue_capacity"] = proxy_config.send_queue_capacity;
    node["send_queue_timeout"] = proxy_config.send_queue_timeout;
    node["threadmodel_type"] = proxy_config.threadmodel_type;
    node["threadmodel_instance_name"] = proxy_config.threadmodel_instance_name;
    node["selector_name"] = proxy_config.selector_name;
    node["namespace"] = proxy_config.namespace_;
    node["load_balance_name"] = proxy_config.load_balance_name;
    node["load_balance_type"] = proxy_config.load_balance_type;
    node["disable_servicerouter"] = proxy_config.disable_servicerouter;
    node["callee_name"] = proxy_config.callee_name;
    node["callee_set_name"] = proxy_config.callee_set_name;
    node["stream_max_window_size"] = proxy_config.stream_max_window_size;
    node["filter"] = proxy_config.service_filters;

    auto& filter_configs = proxy_config.service_filter_configs;
    auto iter = filter_configs.find(trpc::kRetryHedgingLimitFilter);
    if (iter != filter_configs.end()) {
      node["filter_config"][iter->first] = std::any_cast<trpc::RetryHedgingLimitConfig>(iter->second);
    }

    node["ssl"] = proxy_config.ssl_config;

    if (proxy_config.redis_conf.enable) {
      node["redis"] = proxy_config.redis_conf;
    }

    node["fiber_connpool_shards"] = proxy_config.fiber_connpool_shards;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::ServiceProxyConfig& proxy_config) {
    if (node["name"]) proxy_config.name = node["name"].as<std::string>();
    if (node["target"]) proxy_config.target = node["target"].as<std::string>();
    if (node["protocol"]) proxy_config.protocol = node["protocol"].as<std::string>();
    if (proxy_config.protocol == "http") proxy_config.max_packet_size = 0;
    if (node["network"]) proxy_config.network = node["network"].as<std::string>();
    if (node["conn_type"]) proxy_config.conn_type = node["conn_type"].as<std::string>();
    if (node["is_conn_complex"]) proxy_config.is_conn_complex = node["is_conn_complex"].as<bool>();
    if (node["support_pipeline"]) proxy_config.support_pipeline = node["support_pipeline"].as<bool>();
    if (node["fiber_pipeline_connector_queue_size"]) {
      auto queue_size = node["fiber_pipeline_connector_queue_size"].as<uint32_t>();
      proxy_config.fiber_pipeline_connector_queue_size = queue_size > 0 ? queue_size : 16 * 1024;
    }
    if (node["connect_timeout"]) proxy_config.connect_timeout = node["connect_timeout"].as<uint32_t>();
    if (node["timeout"]) proxy_config.timeout = node["timeout"].as<uint32_t>();
    if (node["request_timeout_check_interval"]) {
      auto interval = node["request_timeout_check_interval"].as<uint32_t>();
      proxy_config.request_timeout_check_interval = interval > 0 ? interval : 10;
    }
    if (node["is_reconnection"]) proxy_config.is_reconnection = node["is_reconnection"].as<bool>();
    if (node["allow_reconnect"]) proxy_config.allow_reconnect = node["allow_reconnect"].as<bool>();
	if (node["max_packet_size"]) proxy_config.max_packet_size = node["max_packet_size"].as<uint32_t>();
    if (node["max_conn_num"]) proxy_config.max_conn_num = node["max_conn_num"].as<uint32_t>();
    if (node["idle_time"]) proxy_config.idle_time = node["idle_time"].as<uint32_t>();
    if (node["recv_buffer_size"]) proxy_config.recv_buffer_size = node["recv_buffer_size"].as<uint32_t>();
    if (node["send_queue_capacity"]) proxy_config.send_queue_capacity = node["send_queue_capacity"].as<uint32_t>();
    if (node["send_queue_timeout"]) proxy_config.send_queue_timeout = node["send_queue_timeout"].as<uint32_t>();
    if (node["threadmodel_type"]) proxy_config.threadmodel_type = node["threadmodel_type"].as<std::string>();
    if (node["threadmodel_instance_name"])
      proxy_config.threadmodel_instance_name = node["threadmodel_instance_name"].as<std::string>();
    if (node["selector_name"]) proxy_config.selector_name = node["selector_name"].as<std::string>();
    if (node["namespace"]) proxy_config.namespace_ = node["namespace"].as<std::string>();
    if (node["load_balance_name"]) proxy_config.load_balance_name = node["load_balance_name"].as<std::string>();
    if (node["load_balance_type"]) proxy_config.load_balance_type = node["load_balance_type"].as<std::string>();
    if (node["disable_servicerouter"]) proxy_config.disable_servicerouter = node["disable_servicerouter"].as<bool>();
    if (node["callee_name"]) proxy_config.callee_name = node["callee_name"].as<std::string>();
    if (node["callee_set_name"]) proxy_config.callee_set_name = node["callee_set_name"].as<std::string>();
    if (node["stream_max_window_size"]) {
      proxy_config.stream_max_window_size = node["stream_max_window_size"].as<uint32_t>();
    }
    if (node["ssl"]) {
      proxy_config.ssl_config = node["ssl"].as<trpc::ClientSslConfig>();
    }

    if (node["filter"]) proxy_config.service_filters = node["filter"].as<std::vector<std::string>>();

    if (node["filter_config"]) {
      if (node["filter_config"][trpc::kRetryHedgingLimitFilter]) {
        auto retry_hedging_config =
            node["filter_config"][trpc::kRetryHedgingLimitFilter].as<trpc::RetryHedgingLimitConfig>();
        proxy_config.service_filter_configs[trpc::kRetryHedgingLimitFilter] = retry_hedging_config;
      }
    }

    if (node["redis"]) {
      proxy_config.redis_conf = node["redis"].as<trpc::RedisClientConf>();
      proxy_config.redis_conf.enable = true;
    }

    if (node["fiber_connpool_shards"]) {
      proxy_config.fiber_connpool_shards = node["fiber_connpool_shards"].as<uint32_t>();
    }

    return true;
  }
};

template <>
struct convert<trpc::ClientConfig> {
  static YAML::Node encode(const trpc::ClientConfig& client_config) {
    YAML::Node node;

    node["service"] = client_config.service_proxy_config;

    node["filter"] = client_config.filters;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::ClientConfig& client_config) {
    if (node["service"]) {
      for (size_t idx = 0; idx < node["service"].size(); ++idx) {
        auto item = node["service"][idx].as<trpc::ServiceProxyConfig>();
        client_config.service_proxy_config.push_back(item);
      }
    }

    auto filter = node["filter"];
    if (filter) {
      for (auto&& idx : filter) {
        client_config.filters.push_back(idx.as<std::string>());
      }
    }

    return true;
  }
};

}  // namespace YAML
