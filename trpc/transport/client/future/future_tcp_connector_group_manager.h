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

#include <memory>
#include <unordered_map>

#include "trpc/transport/client/future/conn_complex/future_conn_complex_message_timeout_handler.h"
#include "trpc/transport/client/future/future_connector_group_manager.h"

namespace trpc {

/// @brief Connector group manager for future tcp.
class FutureTcpConnectorGroupManager : public FutureConnectorGroupManager {
 public:
  /// @brief Constructor.
  explicit FutureTcpConnectorGroupManager(const Options& options);

  /// @brief Stop resources.
  void Stop() override;

  /// @brief Destroy resources.
  void Destroy() override;

  /// @brief Allocate connector.
  bool GetOrCreateConnector(const NodeAddr& node_addr, Promise<uint64_t>& promise) override;

  /// @brief Get connector group by backend address.
  FutureConnectorGroup* GetConnectorGroup(const NodeAddr& node_addr) override;

  /// @brief Delete connector group by backend address.
  void DelConnectorGroup(const NodeAddr& node_addr) override;

  /// @brief Disconnect all the connections to a target backend.
  void Disconnect(const std::string& target_ip) override;

  /// @brief Get all the tcp connector group.
  const auto& GetConnectorGroups() { return connector_groups_; }

 private:
  bool CreateTimer();

 private:
  // map from node to connector group.
  std::unordered_map<std::string, std::unique_ptr<FutureConnectorGroup>> connector_groups_;

  // shared msg_timeout_handler for transport adapter when connection complex.
  std::unique_ptr<FutureConnComplexMessageTimeoutHandler> shared_msg_timeout_handler_{nullptr};

  // shared timer id for msg_timeout_handler when connection complex.
  uint64_t timer_id_{kInvalidTimerId};

  // whether timer has created
  bool is_create_timer_{false};
};

}  // namespace trpc
