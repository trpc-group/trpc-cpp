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

#include <any>
#include <deque>
#include <memory>
#include <string>
#include <utility>

#include "trpc/transport/client/common/client_transport_state.h"
#include "trpc/transport/client/future/common/future_connection_handler.h"
#include "trpc/transport/client/future/common/future_connector.h"
#include "trpc/transport/client/future/common/future_connector_options.h"
#include "trpc/transport/client/future/common/utils.h"
#include "trpc/transport/client/future/conn_complex/future_conn_complex_message_timeout_handler.h"
#include "trpc/util/function.h"

namespace trpc {

/// @brief Connection handler for tcp conn complex.
class FutureTcpConnComplexConnectionHandler : public FutureConnectionHandler {
 public:
  // Constructor.
  FutureTcpConnComplexConnectionHandler(const FutureConnectorOptions& options,
                                        FutureConnComplexMessageTimeoutHandler& handler)
    : FutureConnectionHandler(options, options.group_options->trans_info),
      msg_timeout_handler_(handler) {}

  /// @brief Callback to handle response message.
  /// @param conn Related connection.
  /// @param rsp_list Response.
  /// @return true: success, false: failed.
  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& rsp_list) override;

  /// @brief Callback to clean resource when connection closed.
  void CleanResource() override;

  /// @brief Callback when message sent failed.
  /// @param msg Message to sent.
  /// @param code Error code.
  void NotifyMessageFailed(const IoMessage& msg, ConnectionErrorCode code) override;

  /// @brief Set function to delete connection.
  /// @param del_conn_func Function to delete connection.
  void SetDelConnectionFunc(Function<void()>&& del_conn_func) {
    del_conn_func_ = std::move(del_conn_func);
  }

 private:
  // Function to delete connection.
  Function<void()> del_conn_func_ = nullptr;

  // Timeout handler.
  FutureConnComplexMessageTimeoutHandler& msg_timeout_handler_;
};

/// @brief Connection handler for udp io complex.
class FutureUdpIoComplexConnectionHandler final : public FutureConnectionHandler {
 public:
  /// @brief Constructor.
  FutureUdpIoComplexConnectionHandler(const FutureConnectorOptions& options,
                                      FutureConnComplexMessageTimeoutHandler& handler)
    : FutureConnectionHandler(options, options.group_options->trans_info),
      msg_timeout_handler_(handler) {}

  /// @brief Callback to handle response message.
  /// @param conn Related connection.
  /// @param rsp_list Response.
  /// @return true: success, false: failed.
  bool HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& rsp_list) override;

  /// @brief Callback when message sent failed.
  /// @param msg Message to sent.
  /// @param code Error code.
  void NotifyMessageFailed(const IoMessage& msg, ConnectionErrorCode code) override;

 private:
  // Timeout handler.
  FutureConnComplexMessageTimeoutHandler& msg_timeout_handler_;
};

namespace stream {

/// @brief The implementation of future client stream connection handler.
using FutureClientStreamConnComplexConnectionHandler = FutureTcpConnComplexConnectionHandler;

}  // namespace stream

}  // namespace trpc
