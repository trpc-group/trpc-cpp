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
#include <deque>
#include <memory>

#include "trpc/runtime/iomodel/reactor/default/udp_transceiver.h"
#include "trpc/transport/client/future/common/future_connection_handler.h"
#include "trpc/transport/client/future/common/future_connector.h"
#include "trpc/transport/client/future/common/future_connector_options.h"
#include "trpc/transport/client/future/conn_complex/future_conn_complex_message_timeout_handler.h"

namespace trpc {

/// @brief Connector for udp io complex.
class FutureUdpIoComplexConnector : public FutureConnector {
 public:
  explicit FutureUdpIoComplexConnector(FutureConnectorOptions&& options);

  /// @brief Init connector.
  /// @note  Called only once.
  bool Init() override;

  /// @brief Stop connector.
  void Stop() override;

  /// @brief Destroy connector.
  void Destroy() override;

  /// @brief Send request over connector and expect response.
  int SendReqMsg(CTransportReqMsg* req_msg) override;

  /// @brief Send request over connector, not expect response.
  int SendOnly(CTransportReqMsg* req_msg) override;

  /// @brief Check request timeout.
  void HandleRequestTimeout();

  /// @brief Get connection state.
  ConnectionState GetConnectionState() override { return ConnectionState::kUnconnected; }

 private:
  /// @brief Internal implementation of SendReqMsg.
  int SendReqMsgImpl(CTransportReqMsg* req_msg);

 private:
  // Timeout handler.
  FutureConnComplexMessageTimeoutHandler msg_timeout_handler_;

  // Udp transceiver.
  RefPtr<UdpTransceiver> connection_{nullptr};

  // Timer id for detecting request timeout.
  uint64_t timer_id_{kInvalidTimerId};
};

}  // namespace trpc
