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
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "trpc/common/config/default_value.h"
#include "trpc/common/config/mysql_client_conf.h"
#include "trpc/common/config/redis_client_conf.h"
#include "trpc/common/config/ssl_conf.h"
#include "trpc/naming/common/common_inc_deprecated.h"
#include "trpc/runtime/iomodel/reactor/common/connection_handler.h"
#include "trpc/runtime/iomodel/reactor/common/socket.h"
#include "trpc/transport/client/trans_info.h"

namespace trpc {

class ClientContext;
using ClientContextPtr = RefPtr<ClientContext>;
/// Function to handle invoke timeout event.
using ClientTimeoutHandleFunction = std::function<void(const ClientContextPtr&)>;

class ClientTransport;
/// Function to create a client transport, used for scenarios that require custom transport, such as network mocking.
using CreateTransportFunction = std::function<std::unique_ptr<ClientTransport>()>;

/// @brief Callback function, it can be specified by user.
struct ProxyCallback {
  /// callback function when connection establish
  ConnectionEstablishFunction conn_establish_function = nullptr;

  /// callback function when connection closed
  ConnectionCloseFunction conn_close_function = nullptr;

  /// packet integrity verification operation
  ProtocolCheckerFunction checker_function = nullptr;

  /// dispatch strategy for handling response packets
  TransInfo::RspDispatchFunction rsp_dispatch_function = nullptr;

  /// response decoding function
  TransInfo::RspDecodeFunction rsp_decode_function = nullptr;

  /// timeout handle function
  ClientTimeoutHandleFunction client_timeout_handle_function = nullptr;

  /// custom function to create a client transport
  CreateTransportFunction create_transport_function = nullptr;

  /// custom function for setting connection socket options
  SetSocketOptFunction set_socket_opt_function = nullptr;
};

/// @brief Options of service proxy
struct ServiceProxyOption {
  /// The unique identifier for the service_proxy, corresponding to the 'name' parameter passed into TrpcClient's
  /// GetProxy method.
  std::string name;

  /// The name of the called service, which can take on multiple forms:
  /// when directly connecting via IP, it is represented as a group of IP addresses and ports separated by commas, such
  /// as '23.9.0.1:90,34.5.6.7:90',
  /// when using a domain name，it is represented as 'domain name:port', such as 'www.qq.com:80',
  /// when using a name service, it is the name of the service registered on the name service.
  std::string target;

  /// The name of codec used by the service.
  std::string codec_name{kDefaultProtocol};

  /// The network protocol used by the called service, either TCP or UDP.
  std::string network{kDefaultNetwork};

  /// The connection type, either long or short.
  std::string conn_type{kDefaultConnType};

  /// The service-level timeout in milliseconds.
  uint32_t timeout{kDefaultTimeout};

  /// The name of selector plugin used by the service.
  std::string selector_name{kDefaultSelectorName};

  /// The name of caller.
  std::string caller_name;

  /// The name of callee service, used for metrics and tracing report.
  /// If empty, it will be initialized to the name of service proxy.
  std::string callee_name;

  /// The following four fields are deprecated for use with the polaris selector plugin (they are currently internally
  /// compatible within the plugin).
  /// It is recommended to set these fields through code in the service_filter_configs.
  /// The namespace of the service on the polaris naming service, has deprecated.
  /// @private
  std::string name_space;

  /// The set name of the callee service, has deprecated.
  /// @private
  std::string callee_set_name;

  /// Whether to enable set force, has deprecated.
  /// @private
  bool enable_set_force{kDefaultEnableSetForce};

  /// Whether to disable service rule router, has deprecated.
  /// @private
  bool disable_servicerouter{kDefaultDisableServiceRouter};

  /// The name of the load balancing plugin been used. If empty, the default load balancing algorithm will be used.
  std::string load_balance_name;

  /// The type of load balancing plugin used for the polaris selector plugin.
  /// It's deprecated, please use load_balance_name instead.
  /// @private
  LoadBalanceType load_balance_type = LoadBalanceType::DEFAULT;

  /// Whether to use connection complex.
  bool is_conn_complex{kDefaultIsConnComplex};

  /// The maximum packet size that can be received.
  uint32_t max_packet_size{kDefaultMaxPacketSize};

  /// The length of the receive buffer.
  uint32_t recv_buffer_size{kDefaultRecvBufferSize};

  /// The size limit of the send queue.
  uint32_t send_queue_capacity{kDefaultSendQueueCapacity};

  /// The timeout for data to wait to enter the send queue.
  uint32_t send_queue_timeout{kDefaultSendQueueTimeout};

  /// The maximum number of connections that can be established to the backend nodes.
  uint32_t max_conn_num{kDefaultMaxConnNum};

  /// The timeout for idle connections.
  uint32_t idle_time{kDefaultIdleTime};

  /// The interval for request timeout detection, in milliseconds.
  /// It's used in IO/Handle separation and merging mode.
  uint32_t request_timeout_check_interval{kDefaultRequestTimeoutCheckInterval};

  /// Whether to reconnect after the idle connection is disconnected when reach connection idle timeout.
  /// It's used in connection complex under IO/Handle separation and merging mode.
  bool is_reconnection{kDefaultIsReconnection};

  /// The duration of connection timeout in milliseconds. Zero means no connection timeout checking.
  /// Note: it's supported only in IO/Handle separation and merging mode.
  uint32_t connect_timeout{kDefaultConnectTimeout};

  /// Whether to support pipeline.
  bool support_pipeline{kDefaultSupportPipeline};

  /// The queue size of FiberPipelineConnector
  /// if memory usage high, reduce it
  uint32_t fiber_pipeline_connector_queue_size{16 * 1024};

  /// The hashmap bucket size for storing ip/port <--> Connector
  uint32_t endpoint_hash_bucket_size{kEndpointHashBucketSize};

  /// Whether to support reconnection in fixed connection mode, the default value is true.
  /// For scenarios where reconnection is not allowed, such as transactional operations, set this value to false.
  bool allow_reconnect{kDefaultAllowReconnect};

  /// The name of the thread model type, deprecated.
  std::string threadmodel_type_name;

  /// The name of the thread model instance.
  std::string threadmodel_instance_name;

  /// The list of service-level filters.
  std::vector<std::string> service_filters;

  /// The configuration of the service filter, where the key is the name of the filter.
  std::map<std::string, std::any> service_filter_configs;

  /// The callback functions related to request processing.
  ProxyCallback proxy_callback;

  /// The configuration information for Redis authentication.
  RedisClientConf redis_conf;

  /// The configuration information for MySQL authentication.
  MysqlClientConf mysql_conf;

  /// The configuration information for SSL/TLS authentication.
  ClientSslConfig ssl_config;

  /// The size of the sliding window for flow control, in bytes. The default value is 65535, and a value of 0 means that
  /// flow control is disabled. Currently, flow control is effective for tRPC streaming.
  uint32_t stream_max_window_size{kDefaultStreamMaxWindowSize};

  /// The number of FiberConnectionPool shard groups for the idle queue.
  /// A larger value of this parameter will result in a higher allocation of connections, leading to better parallelism
  /// and improved performance. However, it will also result in more connections being created
  /// If you are sensitive to the number of created connections, you may consider reducing this value, such as setting
  /// it to 1
  uint32_t fiber_connpool_shards = 4;
};

}  // namespace trpc
