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
#include <memory>
#include <string>
#include <unordered_map>

#include "trpc/codec/protocol.h"
#include "trpc/filter/filter_base.h"
#include "trpc/runtime/iomodel/reactor/common/accept_connection_info.h"
#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/runtime/iomodel/reactor/common/connection_handler.h"
#include "trpc/runtime/iomodel/reactor/common/io_handler.h"
#include "trpc/runtime/iomodel/reactor/default/acceptor.h"
#include "trpc/transport/server/server_transport_message.h"

#ifdef TRPC_BUILD_INCLUDE_SSL
#include "trpc/transport/common/ssl/ssl.h"
#endif

namespace trpc {

// The function of executing TransportMessage filter
using RunServerFiltersFunction = std::function<FilterStatus(FilterPoint, STransportRspMsg*)>;
// The function of executing io filter
using RunIoFiltersFunction = std::function<FilterStatus(FilterPoint, const std::any&)>;
// The function of handling Stream RPC
using HandleStreamingRpcFunction = std::function<void(STransportReqMsg* recv)>;
// The function of checking whether is unary data or stream data
using CheckStreamRpcFunction = std::function<bool(const std::string& func_name)>;

/// @brief The bind information(include ip/port and so on) of server transport
/// @note  Only used inside the framework
struct BindInfo {
  std::string socket_type;
  std::string ip;
  bool is_ipv6;
  int port;
  std::string network;
  std::string unix_path;
  std::string protocol;
  uint32_t max_packet_size{10000000};
  uint32_t recv_buffer_size{8192};
  uint32_t send_queue_capacity{0};
  uint32_t send_queue_timeout{3000};
  uint32_t max_conn_num{10000};
  uint32_t idle_time{60000};
  uint32_t accept_thread_num{1};

  // Whether the upper-layer business processing methods has stream rpc methods
  bool has_stream_rpc = false;

  // The function whether accept the connection
  // Generally used for ip black and white list
  AcceptConnectionFunction accept_function{nullptr};
  // The function of dispatch the connection
  DispatchAcceptConnectionFunction dispatch_accept_function{nullptr};

  // For unary message
  // The function of check and extract complete request message
  ProtocolCheckerFunction checker_function{nullptr};
  // The function of handle request message
  MessageHandleFunction msg_handle_function{nullptr};

  // For stream message
  // The function of check the message whether is unary or stream message
  CheckStreamRpcFunction check_stream_rpc_function{nullptr};
  // The function of handle stream rpc
  HandleStreamingRpcFunction handle_streaming_rpc_function{nullptr};

  // The function of notify the connection has established
  ConnectionEstablishFunction conn_establish_function{nullptr};
  // The function of notify the connection has closed
  ConnectionCloseFunction conn_close_function{nullptr};

  // The function of send transport message filter
  RunServerFiltersFunction run_server_filters_function{nullptr};
  // The function of send io data filter
  RunIoFiltersFunction run_io_filters_function{nullptr};

  // The function of set socket option for connection socket
  SetSocketOptFunction custom_set_socket_opt_function{nullptr};
  // The function of set socket option for listen socket
  SetSocketOptFunction custom_set_accept_socket_opt_function{nullptr};

  // SSL/TLS
#ifdef TRPC_BUILD_INCLUDE_SSL
  // Option of ssl context: certificate and key and ciphers were stored in.
  std::optional<ssl::ServerSslOptions> ssl_options{};
  // Context of ssl
  ssl::SslContextPtr ssl_ctx{nullptr};
#endif
};

}  // namespace trpc
