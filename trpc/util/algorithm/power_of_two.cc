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

#include "trpc/util/algorithm/power_of_two.h"

namespace trpc {

std::size_t RoundUpPowerOf2(std::size_t n) {
  bool size_is_power_of_2 = (n >= 2) && ((n & (n - 1)) == 0);
  if (!size_is_power_of_2) {
    uint64_t tmp = 1;
    while (tmp <= n) {
      tmp <<= 1;
    }

    n = tmp;
  }
  return n;
}

}  // namespace trpc
