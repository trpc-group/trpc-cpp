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

/// @brief Connector for tcp connection complex.
class FutureTcpConnComplexConnector : public FutureConnector {
 public:
  using ReconnectionHandle = Function<void()>;

  /// @brief Constructor.
  FutureTcpConnComplexConnector(FutureConnectorOptions&& options, FutureConnComplexMessageTimeoutHandler& handler);

  /// @brief Init connector.
  bool Init() override;

  /// @brief Stop connector.
  void Stop() override;

  /// @brief Destroy connector.
  void Destroy() override;

  /// @brief Send request over connector and expect response.
  int SendReqMsg(CTransportReqMsg* req_msg) override;

  /// @brief Send request over connector, not expect response.
  int SendOnly(CTransportReqMsg* req_msg) override;

  /// @brief Handle function when connection idle for a certain time.
  void HandleIdleConnection(uint64_t now_ms);

  /// @brief Get connection state.
  ConnectionState GetConnectionState() override { return future::GetConnectionState(connection_.Get()); }

  /// @brief Create stream over connection.
  stream::StreamReaderWriterProviderPtr CreateStream(stream::StreamOptions&& stream_options) override;

 private:
  /// @brief Set reconnect handle function.
  void SetReconnectionHandle(ReconnectionHandle&& handle) { reconn_handle_ = std::move(handle); }

  /// @brief Send request when connection is connected.
  int SendReqMsgWhenIsConnected(CTransportReqMsg* req_msg);

  /// @brief Internal entry for send request.
  int SendReqMsgInternal(CTransportReqMsg* req_msg);

  /// @brief Do connect.
  bool Connect();

  /// @brief Create a new connection.
  RefPtr<TcpConnection> CreateConnection();

  /// @brief Dispatch reponse to handle thread.
  inline void DispatchResponse(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) {
    future::DispatchResponse(req_msg, rsp_msg, options_.group_options->trans_info->rsp_dispatch_function,
                             options_.group_options->trans_info->run_client_filters_function);
  }

  /// @brief Dispatch exception to handle thread.
  inline void DispatchException(CTransportReqMsg* req_msg, int ret, std::string&& err_msg) {
    future::DispatchException(req_msg, ret, std::move(err_msg),
                              options_.group_options->trans_info->rsp_dispatch_function);
  }

 private:
  // Tcp connection
  RefPtr<TcpConnection> connection_{nullptr};

  // Timeout handler.
  FutureConnComplexMessageTimeoutHandler& msg_timeout_handler_;

  // Reconnect handle function.
  ReconnectionHandle reconn_handle_;

  // Interval between reconnects.
  uint64_t connect_interval_;

  // Connection state.
  std::atomic<ClientTransportState> connection_state_{ClientTransportState::kUnknown};
};

}  // namespace trpc
