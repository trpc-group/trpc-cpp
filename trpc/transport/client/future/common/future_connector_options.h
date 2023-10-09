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

#include "trpc/runtime/iomodel/reactor/common/network_address.h"
#include "trpc/runtime/iomodel/reactor/reactor.h"
#include "trpc/transport/client/trans_info.h"

namespace trpc {

class FutureConnectorGroupManager;

/// @brief Atributes of future connector group.
struct FutureConnectorGroupOptions {
  Reactor* reactor = nullptr;
  TransInfo* trans_info = nullptr;
  // Backend addresss.
  NetworkAddress peer_addr;
  // Ip type of backend, ipv4/ipv6, used by udp.
  NetworkAddress::IpType ip_type;
  // Which manager belongs to.
  FutureConnectorGroupManager* conn_group_manager = nullptr;
};

/// @brief Atributes of future connector.
struct FutureConnectorOptions {
  // Which future connector group options belongs to.
  struct FutureConnectorGroupOptions* group_options = nullptr;
  // Connection id.
  uint64_t conn_id = 0;
  // To decide whether recycle connection after request finish.
  bool is_fixed_connection = false;
};

}  // namespace trpc
