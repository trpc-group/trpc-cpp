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

#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include "trpc/transport/client/client_transport_message.h"
#include "trpc/transport/client/common/client_transport_state.h"
#include "trpc/transport/client/fiber/fiber_connector_group.h"
#include "trpc/transport/client/trans_info.h"
#include "trpc/util/concurrency/lightly_concurrent_hashmap.h"
#include "trpc/util/hazptr/hazptr.h"
#include "trpc/util/hazptr/hazptr_object.h"

namespace trpc {

/// @brief FiberConnectorGroup manager
/// Each `NodeAdder` corresponds to a `FiberConnectorGroup`
class FiberConnectorGroupManager {
 public:
  explicit FiberConnectorGroupManager(TransInfo&& trans_info);

  /// @brief Get `FiberConnectorGroup` by `NodeAddr`, if not existed, create it.
  FiberConnectorGroup* Get(const NodeAddr& node_addr);

  void Stop();

  void Destroy();

 private:
  FiberConnectorGroup* GetFromUdpGroup(const NodeAddr& node_addr);
  FiberConnectorGroup* GetFromTcpGroup(const NodeAddr& node_addr);
  FiberConnectorGroup* CreateTcpConnectorGroup(const NodeAddr& node_addr);
  FiberConnectorGroup* CreateUdpConnectorGroup(bool is_ipv6);
  int GetUdpConnectorGroupIndex(bool is_ipv6) const { return is_ipv6 ? 1 : 0; }

 private:
  struct UdpImpl : HazptrObject<UdpImpl> {
    // 0: ipv4, 1: ipv6
    FiberConnectorGroup* udp_connector_groups[2];
  };

  concurrency::LightlyConcurrentHashMap<std::string, FiberConnectorGroup*> tcp_connector_groups_;

  // Initialized by Stop function to store connector groups which will by destroyed by Destroy function.
  std::unordered_map<std::string, FiberConnectorGroup*> tcp_connector_groups_to_destroy_;

  std::atomic<UdpImpl*> udp_impl_{nullptr};

  std::mutex udp_mutex_;

  std::atomic<ClientTransportState> fiber_transport_state_{ClientTransportState::kUnknown};

  TransInfo trans_info_;
};

}  // namespace trpc
