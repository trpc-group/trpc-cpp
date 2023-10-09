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

#include "trpc/tvar/common/fast_rand.h"

#include <limits>

#include "trpc/tvar/common/op_util.h"

namespace {

using SplitMix64Seed = uint64_t;

/// @brief Store fast created seed.
struct FastRandSeed {
  uint64_t s[2];
};

/// @brief Fast and good enough for normal random scenario.
inline uint64_t SplitMix64Next(SplitMix64Seed* seed) {
  uint64_t z = (*seed += UINT64_C(0x9E3779B97F4A7C15));
  z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
  return z ^ (z >> 31);
}

/// @brief Initialize the seed.
void InitFastRandSeed(FastRandSeed* seed) {
  SplitMix64Seed seed4seed = trpc::tvar::GetSystemMicroseconds();
  seed->s[0] = SplitMix64Next(&seed4seed);
  seed->s[1] = SplitMix64Next(&seed4seed);
}

/// @brief Referenced to http://vigna.di.unimi.it/ftp/papers/xorshiftplus.pdf.
inline uint64_t XorShift128Next(FastRandSeed* seed) {
  uint64_t s1 = seed->s[0];
  const uint64_t s0 = seed->s[1];
  seed->s[0] = s0;
  s1 ^= s1 << 23;
  seed->s[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
  return seed->s[1] + s0;
}

/// @brief Separating uint64_t values into following intervals:
///        [0,range-1][range,range*2-1] ... [uint64_max/range*range,uint64_max]
///        If the generated 64-bit random value falls into any interval except the
///        last one, the probability of taking any value inside [0, range-1] is
///        same. If the value falls into last interval, we retry the process until
///        the value falls into other intervals. If min/max are limited to 32-bits,
///        the retrying is rare. The amortized retrying count at maximum is 1 when
///        range equals 2^32. A corner case is that even if the range is power of
///        2(e.g. min=0 max=65535) in which case the retrying can be avoided, we
///        still retry currently. The reason is just to keep the code simpler
///        and faster for most cases.
inline uint64_t FastRandImpl(uint64_t range, FastRandSeed* seed) {
  uint64_t result;
  const uint64_t div = std::numeric_limits<uint64_t>::max() / range;
  do {
    result = XorShift128Next(seed) / div;
  } while (result >= range);

  return result;
}

/// @brief Tell if the seed is uninitialized.
inline bool NeedInit(const FastRandSeed& seed) {
  return seed.s[0] == 0 && seed.s[1] == 0;
}

thread_local FastRandSeed thread_local_seed = {{0, 0}};

}  // namespace

namespace trpc::tvar {

uint64_t FastRandLessThan(uint64_t range) {
  if (range == 0) {
    return 0;
  }
  if (NeedInit(thread_local_seed)) {
    InitFastRandSeed(&thread_local_seed);
  }
  return FastRandImpl(range, &thread_local_seed);
}

}  // namespace trpc::tvar
