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

#include "trpc/naming/common/util/circuit_break/circuit_break_whitelist.h"

#include "trpc/codec/trpc/trpc.pb.h"

namespace trpc::naming {

CircuitBreakWhiteList::CircuitBreakWhiteList() {
  // Add default error code to whitelist
  circuitbreak_whitelist_.Writer().insert(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR);
  circuitbreak_whitelist_.Writer().insert(TrpcRetCode::TRPC_SERVER_LIMITED_ERR);
  circuitbreak_whitelist_.Swap();
}

void CircuitBreakWhiteList::SetCircuitBreakWhiteList(const std::vector<int>& retcodes) {
  auto& writer = circuitbreak_whitelist_.Writer();
  writer.clear();
  writer.insert(retcodes.begin(), retcodes.end());
  circuitbreak_whitelist_.Swap();
}

bool CircuitBreakWhiteList::Contains(int retcode) {
  auto& reader = circuitbreak_whitelist_.Reader();
  if (reader.find(retcode) != reader.end()) {
    return true;
  }

  return false;
}

}  // namespace trpc::naming
