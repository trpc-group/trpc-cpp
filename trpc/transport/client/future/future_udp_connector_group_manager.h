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

#include "trpc/transport/client/future/future_connector_group_manager.h"

namespace trpc {

/// @brief Connector group manager for future udp.
class FutureUdpConnectorGroupManager : public FutureConnectorGroupManager {
 public:
  /// @brief Constructor.
  explicit FutureUdpConnectorGroupManager(const Options& options) : FutureConnectorGroupManager(options) {}

  /// @brief Stop resources.
  void Stop() override;

  /// @brief Destroy resources.
  void Destroy() override;

  /// @note Fixed connector is not supported in udp.
  bool GetOrCreateConnector(const NodeAddr& node_addr, Promise<uint64_t>& promise) override {
    promise.SetException(CommonException("No support fixed connector in udp", TrpcRetCode::TRPC_CLIENT_NETWORK_ERR));
    return false;
  }

  /// @brief Get connector group by backend address.
  FutureConnectorGroup* GetConnectorGroup(const NodeAddr& node_addr) override;

  /// @brief Delete connector group by backend address.
  /// @note Different backends share same connector group in udp, no need for recycle.
  void DelConnectorGroup(const NodeAddr& node_addr) override {}

  /// @brief Disconnect all the connections to a target backend.
  void Disconnect(const std::string& target_ip) override {}

 private:
  // Different backends in same service share same group, ipv4 apart from ipv6.
  std::unique_ptr<FutureConnectorGroup> udp_connector_group_[2] = {nullptr, nullptr};
};

}  // namespace trpc
