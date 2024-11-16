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
#include <map>
#include <string>
#include <vector>

#include "yaml-cpp/yaml.h"

#include "trpc/common/config/default_value.h"
#include "trpc/common/config/mysql_client_conf.h"
#include "trpc/common/config/redis_client_conf.h"
#include "trpc/common/config/retry_conf.h"
#include "trpc/common/config/ssl_conf.h"
#include "trpc/util/log/logging.h"

namespace trpc {

/// @brief ServiceProxy config
/// `Service level config`, If you want to use `Request-level config`, use `ClientContext`
struct ServiceProxyConfig {
  /// Service name which must be uniqued
  /// Generally its value is the same as `target` when using naming service
  std::string name;

  /// Router name that is registered on the naming service.
  /// Format: trpc.app.server.pb service[.pb interface]
  std::string target;

  /// Application layer protocol
  /// eg: trpc/http/...
  std::string protocol{kDefaultProtocol};

  /// Network type
  /// Support two types: tcp/udp
  std::string network{kDefaultNetwork};

  /// Connection type
  /// Tcp support two types:
  /// 1. long connection
  /// 2. short connection
  std::string conn_type{kDefaultConnType};

  /// Whether support connection multiplex
  /// Connection multiplex means that you can multi-send and multi-recv on one connection and
  /// you need to make sure the protocol supports unique id
  bool is_conn_complex{kDefaultIsConnComplex};

  /// Whether support connection pipeline
  /// Connection pipeline means that you can multi-send and multi-recv in ordered on one connection and
  bool support_pipeline{kDefaultSupportPipeline};

  /// The queue size of FiberPipelineConnector
  /// if memory usage high, reduce it
  uint32_t fiber_pipeline_connector_queue_size = 16 * 1024;

  /// The timeout(ms) of check connection establishment
  /// If set 0, not check
  uint32_t connect_timeout{kDefaultConnectTimeout};

  /// The timeout(ms) of request use by this `ServiceProxy`
  /// `Service level timeout`, If you want to use `Request-level timeout`, use `ClientContext`
  uint32_t timeout{kDefaultTimeout};

  /// The interval(ms) of check request whether has timeout
  uint32_t request_timeout_check_interval{kDefaultRequestTimeoutCheckInterval};

  /// Whether to reconnect after the idle connection is disconnected when reach connection idle timeout.
  bool is_reconnection{kDefaultIsReconnection};

  /// Whether to support reconnection in fixed connection mode, the default value is true.
  /// For scenarios where reconnection is not allowed, such as transactional operations, set this value to false.
  bool allow_reconnect{kDefaultAllowReconnect};

  /// The maximum size of the response packet that the `ServiceProxy` allows to receive
  /// If set 0, disable check th packet size
  uint32_t max_packet_size{kDefaultMaxPacketSize};

  /// The maximum number of connections the `ServiceProxy` allows to establish and cache
  /// If exceed, new connection will be released after used
  uint32_t max_conn_num{kDefaultMaxConnNum};

  /// The timeout(ms) for idle connections
  uint32_t idle_time{kDefaultIdleTime};

  /// When the `ServiceProxy` reads data from the network socket,
  /// the maximum data length allowed to be received at one time
  /// If set 0, not limited
  uint32_t recv_buffer_size{kDefaultRecvBufferSize};

  /// When sending network data, the maximum data length of the io-send queue has cached
  /// Use in fiber runtime, if set 0, not limited
  uint32_t send_queue_capacity{kDefaultSendQueueCapacity};

  /// When sending network data, the timeout(ms) of data in the io-send queue
  /// Use in fiber runtime
  uint32_t send_queue_timeout{kDefaultSendQueueTimeout};

  /// The hashmap bucket size for storing ip/port <--> Connector
  uint32_t endpoint_hash_bucket_size{kEndpointHashBucketSize};

  /// The thread model type use by `ServiceProxy`, deprecated.
  std::string threadmodel_type;

  /// The thread model instance name use by `ServiceProxy`
  /// If only use one thread model in global config, can not be set,
  /// framework will auto use the thread model in global config
  std::string threadmodel_instance_name;

  /// selector name, used by naming plugin
  std::string selector_name;

  /// Namespace, used by naming plugin
  std::string namespace_;

  /// The name of the load balancing plugin used internally by the selector plugin.
  /// If it is empty, the default load balancing strategy will be used.
  std::string load_balance_name;

  /// Only used for the `polaris` selector plugin currently,
  /// to specify the load balancing strategy used by the polaris selector.
  /// This field has been deprecated, you should now directly use `load_balance_name`
  /// to specify the load balancing strategy for it.
  std::string load_balance_type;

  /// Whether to disable service rule-route
  bool disable_servicerouter{kDefaultDisableServiceRouter};

  /// Callee service name, used by metrics/tracing/telemetry plugin
  /// If set empty, it will set with `name`
  std::string callee_name;

  /// Callee set name, specify which set to call
  std::string callee_set_name;

  /// Under streaming, sliding window size(byte) for flow control in recv-side
  /// If set 0, disable flow control
  /// Currently only supports trpc streaming protocol
  uint32_t stream_max_window_size = kDefaultStreamMaxWindowSize;

  /// SSL/TLS config
  ClientSslConfig ssl_config;

  /// The filter name use by this service
  std::vector<std::string> service_filters;

  /// Service filter config, the key is filter name.
  std::map<std::string, std::any> service_filter_configs;

  /// Redis auth config
  RedisClientConf redis_conf;

  /// MySQL auth config
  MysqlClientConf mysql_conf;

  /// The number of FiberConnectionPool shard groups for the idle queue.
  /// A larger value of this parameter will result in a higher allocation of connections, leading to better parallelism
  /// and improved performance. However, it will also result in more connections being created
  /// If you are sensitive to the number of created connections, you may consider reducing this value, such as setting
  /// it to 1
  uint32_t fiber_connpool_shards = 4;

  void Display() const;
};

/// @brief Client config
struct ClientConfig {
  /// Service configuration collection
  std::vector<ServiceProxyConfig> service_proxy_config;

  /// Filters used by all services
  std::vector<std::string> filters;

  void Display() const;
};

}  // namespace trpc
