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

#include "trpc/naming/domain/domain_selector_conf.h"

#include "trpc/util/log/logging.h"

namespace trpc::naming {

void DomainSelectorConfig::Display() const {
  TRPC_FMT_DEBUG("-----DomainSelectorConfig begin-------");

  TRPC_FMT_DEBUG("exclude_ipv6:{}", exclude_ipv6);

  circuit_break_config.Display();

  TRPC_FMT_DEBUG("--------------------------------------");
}

}  // namespace trpc::naming
