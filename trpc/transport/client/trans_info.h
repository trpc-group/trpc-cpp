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
#include <string>

#include "trpc/codec/protocol.h"
#include "trpc/filter/filter.h"
#include "trpc/filter/filter_point.h"
#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/runtime/threadmodel/common/msg_task.h"
#ifdef TRPC_BUILD_INCLUDE_SSL
#include "trpc/transport/common/ssl/ssl.h"
#endif
#include "trpc/runtime/iomodel/reactor/common/io_handler.h"
#include "trpc/util/object_pool/object_pool_ptr.h"

namespace trpc {

class CTransportReqMsg;

struct TransInfo {
  /// The function of dispatch response packet
  using RspDispatchFunction = std::function<bool(object_pool::LwUniquePtr<MsgTask>&& task)>;

  /// The function of decode response
  using RspDecodeFunction = std::function<bool(std::any&& in, ProtocolPtr& out)>;

  /// The function for creating a custom iohandler
  using CustomIoHandlerFunction = std::function<IoHandler*(IoHandler* parent)>;

  /// The filter function for transport
  using RunClientFiltersFunction = std::function<FilterStatus(FilterPoint, CTransportReqMsg*)>;

  /// The filter function for io
  using RunIoFiltersFunction = std::function<FilterStatus(FilterPoint point, const std::any& msg)>;

  /// Connection type
  ConnectionType conn_type;

  /// Max packet size that can recv message
  uint32_t max_packet_size = 10000000;

  /// Max connection num
  uint32_t max_conn_num = 64;

  /// Whether to use connection multiplexing
  bool is_complex_conn = true;

  /// Whether to support pipeline
  bool support_pipeline = false;

  /// Whether to support reconnection after the connection is disconnected (checked when the request is sent),
  /// supported by default, used for fixed connection scenarios(for merge runtime)
  /// If reconnection is not supported, when the fixed connection is disconnected,
  /// sending a packet on this connection will fail; otherwise, it will reconnect and then send a packet
  bool allow_reconnect = true;

  /// Whether to reconnect after the idle connection is disconnected when reach connection idle timeout.
  bool is_reconnection = false;

  /// Connection idle timeout
  uint32_t connection_idle_timeout = 50000;

  /// The interval of check request timeout(ms)
  uint32_t request_timeout_check_interval = 10;

  /// The timeout of establishing connection
  uint32_t connect_timeout = 0;

  /// The length of the receive buffer
  uint32_t recv_buffer_size = 8192;

  /// The size limit of the send queue(for fiber)
  uint32_t send_queue_capacity = 0;

  /// The timeout for data to wait to enter the send queue(for fiber)
  uint32_t send_queue_timeout = 3000;

  /// Protocol name
  std::string protocol;

  /// Whether streaming proxy
  bool is_stream_proxy = false;

  /// The queue size of FiberPipelineConnector
  /// if memory usage high, reduce it
  uint32_t fiber_pipeline_connector_queue_size = 16 * 1024;

  /// The callback function when connection establish
  ConnectionEstablishFunction conn_establish_function = nullptr;

  /// The callback function when connection closed
  ConnectionCloseFunction conn_close_function = nullptr;

  /// The operation of packet integrity verification
  ProtocolCheckerFunction checker_function = nullptr;

  /// The function of dispatch strategy for handling response packets
  RspDecodeFunction rsp_decode_function = nullptr;

  /// The function of dispatch response
  RspDispatchFunction rsp_dispatch_function = nullptr;

  /// The function for creating a custom iohandler
  CustomIoHandlerFunction custom_io_handler_function = nullptr;

  /// The filter function for transport
  RunClientFiltersFunction run_client_filters_function = nullptr;

  /// The filter function for io
  RunIoFiltersFunction run_io_filters_function = nullptr;

  /// The custom function for setting connection socket options
  SetSocketOptFunction set_socket_opt_function = nullptr;

  /// Set SSL/TLS options and context for client.
#ifdef TRPC_BUILD_INCLUDE_SSL
  /// Option of ssl context: certificate and key and ciphers were stored in.
  std::optional<ssl::ClientSslOptions> ssl_options{};
  /// Context of ssl
  ssl::SslContextPtr ssl_ctx = nullptr;
#endif

  /// User-defined transInfo data,such as RedisClientConf
  std::any user_data;

  /// The number of FiberConnectionPool shard groups for the idle queue.
  /// A larger value of this parameter will result in a higher allocation of connections, leading to better parallelism
  /// and improved performance. However, it will also result in more connections being created
  /// If you are sensitive to the number of created connections, you may consider reducing this value, such as setting
  /// it to 1
  uint32_t fiber_connpool_shards = 4;
};

}  // namespace trpc
