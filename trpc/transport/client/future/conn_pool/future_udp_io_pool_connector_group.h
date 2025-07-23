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

#include "trpc/transport/client/future/conn_pool/conn_pool.h"
#include "trpc/transport/client/future/conn_pool/future_udp_io_pool_connector.h"
#include "trpc/transport/client/future/conn_pool/shared_send_queue.h"
#include "trpc/transport/client/future/future_connector_group.h"
#include "trpc/util/function.h"

namespace trpc {

/// @brief Connector group for future udp io pool.
class FutureUdpIoPoolConnectorGroup : public FutureConnectorGroup {
 public:
  /// @brief Constructor.
  explicit FutureUdpIoPoolConnectorGroup(FutureConnectorGroupOptions&& options);

  /// @brief Destructor.
  ~FutureUdpIoPoolConnectorGroup() override = default;

  /// @brief Init connector group.
  bool Init() override;

  /// @brief Stop connector group.
  void Stop() override;

  /// @brief Destroy connector group.
  void Destroy() override;

  /// @note Fixed type connector is not supported in udp.
  bool ReleaseConnector(uint64_t fixed_connector_id) override { return false; }

  /// @note Fixed type connector is not supported in udp.
  bool GetOrCreateConnector(const NodeAddr& node_addr, Promise<uint64_t>& promise) override {
    return false;
  }

  /// @brief Send request.
  int SendReqMsg(CTransportReqMsg* req_msg) override;

  /// @brief Send request, not expect response.
  int SendOnly(CTransportReqMsg* req_msg) override;

 private:
  /// @brief Get connector by connector id, create if necessary.
  FutureUdpIoPoolConnector* GetOrCreateConnector(uint64_t conn_id);

  /// @brief Handle request timeout.
  void HandleReqTimeout();

 private:
  // Connection pool.
  UdpIoPool conn_pool_;

  // Already sent request timeout queue.
  internal::SharedSendQueue shared_send_queue_;

  // Function for request timeout handling in send queue.
  Function<void(const internal::SharedSendQueue::DataIterator&)> timeout_handle_function_;

  // Timer id to detect timeout.
  uint64_t timer_id_{kInvalidTimerId};
};

}  // namespace trpc
