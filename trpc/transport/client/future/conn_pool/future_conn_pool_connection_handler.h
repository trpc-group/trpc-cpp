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

#include <deque>
#include <utility>

#include "trpc/transport/client/future/common/future_connection_handler.h"
#include "trpc/transport/client/future/common/future_connector.h"
#include "trpc/transport/client/future/common/utils.h"
#include "trpc/transport/client/future/conn_pool/conn_pool.h"
#include "trpc/transport/client/future/conn_pool/future_conn_pool_message_timeout_handler.h"
#include "trpc/transport/client/future/conn_pool/future_tcp_conn_pool_connector.h"
#include "trpc/transport/client/future/conn_pool/future_udp_io_pool_connector.h"

namespace trpc {

/// @brief Connection handler for tcp conn pool.
class FutureTcpConnPoolConnectionHandler : public FutureConnectionHandler {
 public:
  /// @note Constructor.
  FutureTcpConnPoolConnectionHandler(const FutureConnectorOptions& options,
                                     FutureConnPoolMessageTimeoutHandler& msg_timeout_handler)
    : FutureConnectionHandler(options, options.group_options->trans_info),
      msg_timeout_handler_(msg_timeout_handler) {}

  /// @brief Callback to handle response message.
  /// @param conn Related connection.
  /// @param rsp_list Response.
  /// @return true: success, false: failed.
  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& rsp_list) override;

  /// @brief Callback to clean resource when connection closed.
  void CleanResource() override;

  /// @brief Get merge request count.
  uint32_t GetMergeRequestCount() override;

  /// @brief Set current sent request message related context.
  /// @note  For framework internal use only.
  void SetCurrentContextExt(uint32_t context_ext) override { context_ext_ = context_ext; }

  /// @brief Get current sent request message related context.
  uint32_t GetCurrentContextExt() override { return context_ext_; }

  /// @brief Set connection pool.
  /// @note  To unify constructor signature for FutureUdpIoPoolConnectionHandler
  ///        and FutureTcpConnPoolConnectionHandler.
  ///        Called after instance created.
  void SetConnPool(TcpConnPool* conn_pool) { conn_pool_ = conn_pool; }

 private:
  bool SendPendingMsg();

 private:
  // Message timeout handler belongs to current connector.
  FutureConnPoolMessageTimeoutHandler& msg_timeout_handler_;

  // Which connection pool current connector belongs to.
  // @note Not ownd by current connection handler.
  TcpConnPool* conn_pool_;

  // Context.
  uint32_t context_ext_;
};

/// @brief Connection handler for udp io pool.
class FutureUdpIoPoolConnectionHandler final : public FutureConnectionHandler {
 public:
  /// @brief Constructor.
  FutureUdpIoPoolConnectionHandler(const FutureConnectorOptions& options,
                                   FutureConnPoolMessageTimeoutHandler& msg_timeout_handler)
    : FutureConnectionHandler(options, options.group_options->trans_info),
      msg_timeout_handler_(msg_timeout_handler) {}

  /// @brief Callback to handle response message.
  /// @param conn Related connection.
  /// @param rsp_list Response.
  /// @return true: success, false: failed.
  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& rsp_list) override;

  /// @brief Get merge request count.
  uint32_t GetMergeRequestCount() override;

  /// @brief Set connection pool.
  /// @note  To unify constructor signature for FutureUdpIoPoolConnectionHandler
  ///        and FutureTcpConnPoolConnectionHandler.
  ///        Called after instance created.
  void SetConnPool(UdpIoPool* conn_pool) { conn_pool_ = conn_pool; }

 private:
  bool SendPendingMsg();

 private:
  // Message timeout handler belongs to current connector.
  FutureConnPoolMessageTimeoutHandler& msg_timeout_handler_;

  // Which connection pool current connector belongs to.
  // @note Not ownd by current connection handler.
  UdpIoPool* conn_pool_;
};

namespace stream {

/// @brief The implementation of future client stream connection handler.
using FutureClientStreamConnPoolConnectionHandler = FutureTcpConnPoolConnectionHandler;

}  // namespace stream

}  // namespace trpc
