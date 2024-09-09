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

#include "trpc/common/config/load_balance_naming_conf.h"

#include "trpc/util/log/logging.h"

namespace trpc::naming {
void SWRoundrobinLoadBalanceConfig::Display() const {
  TRPC_FMT_DEBUG("-----SWRoundrobinLoadBalanceConfig begin-------");

  for (const auto& [name, service] : services) {
    TRPC_FMT_DEBUG("Service name: {}", name);

    for (const auto& [address, weight] : service) {
      TRPC_FMT_DEBUG("  Address: {}, Weight: {}", address, weight);
    }
    TRPC_FMT_DEBUG("-----------------------------------------------");
  }

  TRPC_FMT_DEBUG("-----SWRoundrobinLoadBalanceConfig end---------");
}
}  // namespace trpc::naming
