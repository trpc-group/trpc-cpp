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

#include "trpc/common/config/loadbalance_naming_conf.h"

#include "trpc/util/log/logging.h"

namespace trpc::naming {

void LoadBalanceSelectorConfig::Display() const {
  TRPC_FMT_DEBUG("-----LoadBalanceSelectorConfig begin-------");

  TRPC_FMT_DEBUG("load_balance:{}", load_balance_name);
  TRPC_FMT_DEBUG("hash_nodes:{}", hash_nodes);
  TRPC_FMT_DEBUG("hash_args size:{}", hash_args.size());
  TRPC_FMT_DEBUG("hash_func:{}", hash_func);

  TRPC_FMT_DEBUG("--------------------------------------");
}

}  // namespace trpc::naming