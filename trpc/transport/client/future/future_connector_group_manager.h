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

#include <string>

#include "trpc/runtime/iomodel/reactor/reactor.h"
#include "trpc/transport/client/future/future_connector_group.h"
#include "trpc/transport/client/trans_info.h"

namespace trpc {

/// @brief Class to manage future connector groups.
class FutureConnectorGroupManager {
 public:
  struct Options {
    Reactor* reactor{nullptr};
    TransInfo* trans_info{nullptr};
  };

  /// @brief Constructor.
  FutureConnectorGroupManager(Options options) : options_(options) {}

  /// @brief Destructor.
  virtual ~FutureConnectorGroupManager() = default;

  /// @brief Close connections, timers, etc.
  virtual void Stop() = 0;

  /// @brief Destroy manager.
  virtual void Destroy() = 0;

  /// @brief Allocate connector.
  /// @param[in] node_addr Backend address.
  /// @param[out] promise Result promise.
  /// @return true: success, false: failed.
  virtual bool GetOrCreateConnector(const NodeAddr& node_addr, Promise<uint64_t>& promise) = 0;

  /// @brief Get connector group by backend address.
  /// @param[in] node_addr Backend address.
  /// @return Pointer to ConnectorGroup, nullptr if failed.
  virtual FutureConnectorGroup* GetConnectorGroup(const NodeAddr& node_addr) = 0;

  /// @brief Delete connector group by backend address.
  /// @param[in] node_addr Backend address.
  virtual void DelConnectorGroup(const NodeAddr& node_addr) = 0;

  /// @brief Disconnect all the connections to a target backend.
  /// @param[in] target_ip Target ip.
  virtual void Disconnect(const std::string& target_ip) = 0;

 protected:
  // Connection options.
  Options options_;
};

}  // namespace trpc
