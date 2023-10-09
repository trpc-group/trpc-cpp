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

#include <string>

#include "trpc/runtime/iomodel/reactor/common/connection_handler.h"

namespace trpc {

// Addresss node
struct alignas(8) NodeAddr {
  enum class AddrType : uint8_t { kIpV4, kIpV6 };
  // v4/v6
  AddrType addr_type = AddrType::kIpV4;

  // connection type: tcp/udp
  ConnectionType connection_type = ConnectionType::kTcpLong;

  // peer port
  uint16_t port = 0;

  // peer ip
  std::string ip = "";
};

struct alignas(8) ExtendNodeAddr {
  // IP:Port of remote service instance.
  NodeAddr addr;

  // Metadata of remote service instance for metrics report.
  std::unordered_map<std::string, std::string> metadata;
};

}  // namespace trpc
