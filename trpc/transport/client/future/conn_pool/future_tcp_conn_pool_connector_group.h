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

#include <vector>

#include "trpc/transport/client/future/conn_pool/conn_pool.h"
#include "trpc/transport/client/future/conn_pool/future_tcp_conn_pool_connector.h"
#include "trpc/transport/client/future/conn_pool/shared_send_queue.h"
#include "trpc/transport/client/future/future_connector_group.h"
#include "trpc/util/function.h"

namespace trpc {

/// @brief Connector group for future tcp connection pool.
class FutureTcpConnPoolConnectorGroup : public FutureConnectorGroup {
 public:
  /// @brief Constructor.
  explicit FutureTcpConnPoolConnectorGroup(FutureConnectorGroupOptions&& options);

  /// @brief Destructor.
  ~FutureTcpConnPoolConnectorGroup() override = default;

  /// @brief Init connector group.
  bool Init() override;

  /// @brief Stop connector group.
  void Stop() override;

  /// @brief Destroy connector group.
  void Destroy() override;

  /// @brief Release a fiexed type connector.
  bool ReleaseConnector(uint64_t fixed_connector_id) override;

  /// @brief Get connector, create if necessary.
  bool GetOrCreateConnector(const NodeAddr& node_addr, Promise<uint64_t>& promise) override;

  /// @brief Send request.
  int SendReqMsg(CTransportReqMsg* req_msg) override;

  /// @brief Send request, not expect response.
  int SendOnly(CTransportReqMsg* req_msg) override;

  /// @brief Create stream.
  stream::StreamReaderWriterProviderPtr CreateStream(stream::StreamOptions&& stream_options) override;

 private:
  /// @brief Get connector by connector id, create if necessary.
  FutureTcpConnPoolConnector* GetOrCreateConnector(uint64_t conn_id);

  /// @brief Get connector by fixed id.
  FutureTcpConnPoolConnector* GetConnectorByFixedId(uint64_t fixed_connector_id);

  /// @brief Handle request timeout.
  void HandleReqTimeout();

  /// @brief Handle idle connection.
  void HandleIdleConnection();

 private:
  // Connection pool.
  TcpConnPool conn_pool_;

  // Already sent request timeout queue.
  internal::SharedSendQueue shared_send_queue_;

  // Function for request timeout handling in send queue.
  Function<void(const internal::SharedSendQueue::DataIterator&)> timeout_handle_function_;

  // Timer ids to detect timeout.
  std::vector<uint64_t> timer_ids_;

  // How many fixed-type connections.
  uint32_t fixed_connection_count_{0};
};

}  // namespace trpc
