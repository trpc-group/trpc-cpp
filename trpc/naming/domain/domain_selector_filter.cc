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

#include "trpc/naming/domain/domain_selector_filter.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/naming/domain/domain_selector_conf.h"
#include "trpc/naming/domain/domain_selector_conf_parser.h"

namespace trpc {
DomainSelectorFilter::DomainSelectorFilter() {
  naming::DomainSelectorConfig config;
  if (!TrpcConfig::GetInstance()->GetPluginConfig<naming::DomainSelectorConfig>("selector", "domain", config)) {
    TRPC_FMT_DEBUG("Get plugin config fail, use default config");
  }
  selector_flow_ = std::make_unique<SelectorWorkFlow>("domain", config.circuit_break_config.enable, false);
}
}  // namespace trpc