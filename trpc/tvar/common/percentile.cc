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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. incubator-brpc
// Copyright (C) 2019 The Apache Software Foundation
// incubator-brpc is licensed under the Apache 2.0 License.
//
//

#include "trpc/tvar/common/percentile.h"

namespace {

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/bvar/detail/percentile.cpp

inline uint32_t Ones32(uint32_t x) {
  // 32-bit recursive reduction using SWAR...
  // but first step is mapping 2-bit values
  // into sum of 2 1-bit values in sneaky way.
  x -= ((x >> 1) & 0x55555555);
  x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
  x = (((x >> 4) + x) & 0x0f0f0f0f);
  x += (x >> 8);
  x += (x >> 16);
  return (x & 0x0000003f);
}

inline uint32_t Log2(uint32_t x) {
  int y = (x & (x - 1));
  y |= -y;
  y >>= 31;
  x |= (x >> 1);
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >> 16);
  return (Ones32(x) - 1 - y);
}

inline size_t GetIntervalIndex(uint32_t& x) {
  if (x <= 2) {
    return 0;
  } else if (x > std::numeric_limits<uint32_t>::max()) {
    x = std::numeric_limits<uint32_t>::max();
    return 31;
  } else {
    return Log2(x) - 1;
  }
}

}  // namespace

namespace trpc::tvar {

void AddLatency::operator()(GlobalValue<WriteMostly<PercentileTraits>::WriteBufferWrapper>* global_value,
                            ThreadLocalPercentileSamples* local_value, uint32_t latency) {
  // GetIntervalIndex may change input.
  const size_t index = GetIntervalIndex(latency);
  PercentileInterval<ThreadLocalPercentileSamples::SAMPLE_SIZE>& interval = local_value->GetIntervalAt(index);
  if (interval.Full()) {
    GlobalPercentileSamples* g = global_value->Lock();
    g->GetIntervalAt(index).Merge(interval);
    g->num_added_ += interval.AddedCount();
    global_value->Unlock();
    local_value->num_added_ -= interval.AddedCount();
    interval.Clear();
  }
  // Add must comes before num_added updated.
  interval.Add32(latency);
  ++local_value->num_added_;
}

// End of source codes that are from incubator-brpc.

}  // namespace trpc::tvar
