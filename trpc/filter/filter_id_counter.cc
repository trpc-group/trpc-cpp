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

#include <atomic>
#include <limits>

#include "trpc/filter/filter_id_counter.h"
#include "trpc/util/log/logging.h"

namespace trpc {

// Define the global atomic variable that stores the next available filter ID.
static std::atomic<uint16_t> filter_id_counter{10000};

uint16_t GetNextFilterID() {
  uint16_t next_id = filter_id_counter.fetch_add(1);
  TRPC_ASSERT(next_id < std::numeric_limits<uint16_t>::max() &&
               "filter_id_counter has reached its maximum value: 65535");
  return next_id;
}

// Unit test use only, users are forbidden to use
void SetFilterIDCounter(uint16_t value) {
  filter_id_counter.store(value);
}

} // namespace trpc
