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

#include "trpc/transport/client/future/common/future_connection_handler.h"
#include "trpc/transport/client/future/common/future_connector.h"
#include "trpc/transport/client/future/common/utils.h"
#include "trpc/transport/client/future/conn_pool/conn_pool.h"
#include "trpc/transport/client/future/conn_pool/future_conn_pool_message_timeout_handler.h"

namespace trpc {

class FutureUdpIoPoolConnector;
using UdpIoPool = ConnPool<FutureUdpIoPoolConnector>;

/// @brief Connector for udp io pool.
class FutureUdpIoPoolConnector : public FutureConnector {
 public:
  /// @brief Constructor.
  FutureUdpIoPoolConnector(FutureConnectorOptions&& options,
                           internal::SharedSendQueue& shared_send_queue,
                           UdpIoPool* conn_pool)
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
  void HandleSendQueueTimeout(CTransportReqMsg* req_msg);

  /// @brief Get connection state.
  /// @note  Udp do not need to reconnect, always return kUnconnected.
  ConnectionState GetConnectionState() override { return ConnectionState::kUnconnected; }

 private:
  // Which connection pool belongs to.
  UdpIoPool* conn_pool_;

  // Timeout handler.
  FutureConnPoolMessageTimeoutHandler msg_timeout_handler_;

  // Udp transceiver.
  RefPtr<UdpTransceiver> connection_ = nullptr;
};

}  // namespace trpc
