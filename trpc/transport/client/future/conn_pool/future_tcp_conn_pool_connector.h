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

namespace trpc {

class FutureTcpConnPoolConnector;
using TcpConnPool = ConnPool<FutureTcpConnPoolConnector>;

/// @brief Connector for tcp connection pool.
class FutureTcpConnPoolConnector : public FutureConnector {
 public:
  /// @brief Constructor.
  FutureTcpConnPoolConnector(FutureConnectorOptions&& options,
                             internal::SharedSendQueue& shared_send_queue,
                             TcpConnPool* conn_pool)
    : FutureConnector(std::move(options)),
      conn_pool_(conn_pool),
      msg_timeout_handler_(options_.conn_id,
                           options_.group_options->trans_info->rsp_dispatch_function,
                           shared_send_queue) {}

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

  /// @brief Handle shared send queue timeout.
  void HandleSendQueueTimeout(CTransportReqMsg* msg);

  /// @brief Handle pending queue timeout.
  void HandlePendingQueueTimeout();

  /// @brief Handle function when connection idle for a certain time.
  void HandleIdleConnection(uint64_t now_ms);

  /// @brief Check connection idle or not.
  bool IsConnectionIdle() {
    return msg_timeout_handler_.IsSendQueueEmpty() && msg_timeout_handler_.IsPendingQueueEmpty();
  }

  /// @brief Mark current connector as fixed.
  void SetIsFixedConnection(bool flag) { options_.is_fixed_connection = flag; }

  /// @brief Get current connector fixed or not.
  bool IsFixedConnection() { return options_.is_fixed_connection; }

  /// @brief Get connection state.
  ConnectionState GetConnectionState() override { return future::GetConnectionState(connection_.Get()); }

  /// @brief Get pointer to connection, used by stream.
  Connection* GetConnection() { return connection_.Get(); }

 private:
  /// @brief Get connector id.
  inline uint64_t GetConnectorId() { return options_.conn_id; }

  /// @brief Internal implementation for sending request.
  int SendReqMsgImpl(CTransportReqMsg* req_msg);

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
  // Which connection pool belongs to.
  TcpConnPool* conn_pool_;

  // Timeout handler.
  FutureConnPoolMessageTimeoutHandler msg_timeout_handler_;

  // Tcp connection.
  RefPtr<TcpConnection> connection_ = nullptr;
};

}  // namespace trpc
