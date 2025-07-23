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

#include <cstdint>
#include <memory>

#include "trpc/transport/client/future/conn_complex/future_conn_complex_message_timeout_handler.h"
#include "trpc/transport/client/future/conn_complex/future_tcp_conn_complex_connector.h"
#include "trpc/transport/client/future/future_connector_group.h"

namespace trpc {

/// @brief Connector group for future tcp connection complex.
class FutureTcpConnComplexConnectorGroup : public FutureConnectorGroup {
 public:
  FutureTcpConnComplexConnectorGroup(FutureConnectorGroupOptions&& options,
                                     FutureConnComplexMessageTimeoutHandler& handler);

  /// @brief Init connector group.
  bool Init() override;

  /// @brief Stop connector group.
  void Stop() override;

  /// @brief Destroy connector group.
  void Destroy() override;

  /// @brief Create stream.
  stream::StreamReaderWriterProviderPtr CreateStream(stream::StreamOptions&& stream_options) override;

  /// @brief Release a fiexed type connector.
  bool ReleaseConnector(uint64_t fixed_connector_id) override;

  /// @brief Get connector, create if necessary.
  bool GetOrCreateConnector(const NodeAddr& node_addr, Promise<uint64_t>& promise) override;

  /// @brief Send request.
  int SendReqMsg(CTransportReqMsg* req_msg) override;

  /// @brief Send request, not expect response.
  int SendOnly(CTransportReqMsg* req_msg) override;

 private:
  /// @brief Handle idle connection.
  void HandleIdleConnection();

 private:
  // Tcp connector.
  std::unique_ptr<FutureTcpConnComplexConnector> connector_{nullptr};

  // Timer id to detect idle connections.
  uint64_t idle_connection_timer_id_{kInvalidTimerId};

  // Count of fixed type connection.
  uint32_t fixed_connection_count_{0};
};

}  // namespace trpc
