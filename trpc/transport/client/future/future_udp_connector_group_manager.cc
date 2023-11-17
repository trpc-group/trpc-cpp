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

#include "trpc/transport/client/future/future_udp_connector_group_manager.h"

#include <string_view>

#include "trpc/transport/client/future/common/future_connector_options.h"
#include "trpc/transport/client/future/conn_complex/future_udp_io_complex_connector_group.h"
#include "trpc/transport/client/future/conn_pool/future_udp_io_pool_connector_group.h"
#include "trpc/transport/common/transport_message_common.h"

namespace {

/// @brief Get connector group index by address type.
int GetUdpConnectorGroupIndex(trpc::NodeAddr::AddrType type) {
  return type == trpc::NodeAddr::AddrType::kIpV4 ? 0 : 1;
}

}  // namespace

namespace trpc {

void FutureUdpConnectorGroupManager::Stop() {
  for (auto& item : udp_connector_group_) {
    if (item) item->Stop();
  }
}

void FutureUdpConnectorGroupManager::Destroy() {
  for (auto& item : udp_connector_group_) {
    if (item) item->Destroy();
  }
}

FutureConnectorGroup* FutureUdpConnectorGroupManager::GetConnectorGroup(const NodeAddr& node_addr) {
  auto& udp_connector_group = udp_connector_group_[GetUdpConnectorGroupIndex(node_addr.addr_type)];
  if (TRPC_LIKELY(udp_connector_group != nullptr)) {
    return udp_connector_group.get();
  }

  FutureConnectorGroupOptions connector_group_options;
  connector_group_options.reactor = options_.reactor;
  connector_group_options.trans_info = options_.trans_info;

  if (node_addr.addr_type == NodeAddr::AddrType::kIpV6) {
    connector_group_options.ip_type = NetworkAddress::IpType::kIpV6;
  } else {
    connector_group_options.ip_type = NetworkAddress::IpType::kIpV4;
  }

  if (options_.trans_info->is_complex_conn) {
    udp_connector_group = std::make_unique<FutureUdpIoComplexConnectorGroup>(std::move(connector_group_options));
  } else {
    udp_connector_group = std::make_unique<FutureUdpIoPoolConnectorGroup>(std::move(connector_group_options));
  }

  if (udp_connector_group->Init()) {
    return udp_connector_group.get();
  }

  udp_connector_group = nullptr;

  return nullptr;
}

}  // namespace trpc
