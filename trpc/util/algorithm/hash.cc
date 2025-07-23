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

#include "trpc/util/algorithm/hash.h"

#include "trpc/util/log/logging.h"

namespace trpc {

std::size_t GetHashValue(std::uint64_t x) {
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = (x >> 16) ^ x;
  return x;
}

std::size_t GetHashIndex(std::uint64_t x, std::uint64_t mod) {
  TRPC_ASSERT(mod != 0);
  return GetHashValue(x) % mod;
}

}  // namespace trpc
