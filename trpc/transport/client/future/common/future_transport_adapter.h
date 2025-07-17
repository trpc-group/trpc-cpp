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

#include <memory>

#include "trpc/runtime/iomodel/reactor/reactor.h"
#include "trpc/transport/client/common/client_transport_state.h"
#include "trpc/transport/client/future/future_connector_group_manager.h"
#include "trpc/transport/client/trans_info.h"

namespace trpc {

/// @brief Future transport adapter, different io thread has its own adapter.
class FutureTransportAdapter {
 public:
  /// @brief Connector options.
  struct Options {
    Reactor* reactor{nullptr};
    TransInfo* trans_info{nullptr};
  };

  /// @brief Constructor.
  explicit FutureTransportAdapter(const Options& options);

  /// @brief Destructor.
  ~FutureTransportAdapter();

  /// @brief Get connector group by backend.
  /// @param node_addr Backend address.
  /// @return Pointer to ConnectorGroup, nullptr if failed.
  FutureConnectorGroup* GetConnectorGroup(const NodeAddr& node_addr);

  /// @brief Stop resources.
  void Stop();

  /// @brief Destroy resources.
  void Destroy();

  /// @brief Allocate connector.
  /// @param node_addr Backend address.
  /// @param Promise<uint64_t> Result promise.
  /// @return true: success, false: failed.
  bool GetOrCreateConnector(const NodeAddr& node_addr, Promise<uint64_t>& promise);

  /// @brief Disconnect all the connections to a target backend.
  /// @param target_ip Target ip.
  void Disconnect(const std::string& target_ip);

 private:
  // Adapter options.
  Options options_;

  // Connector group manager.
  std::unique_ptr<FutureConnectorGroupManager> group_manager_{nullptr};

  // State of client transport.
  std::atomic<ClientTransportState> transport_state_{ClientTransportState::kUnknown};
};

}  // namespace trpc
