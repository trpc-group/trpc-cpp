//
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/tsc.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

// Let me be clear: Generally, you SHOULDN'T use TSC as timestamp in the first
// place:
//
// - If you need precise timestamp, consider using `ReadXxxClock()` (@sa:
//   `chrono.h`),
// - If you need to read timestamp fast enough (but can tolerate a lower
//   resolution), use `ReadCoarseXxxClock()`).
//

#pragma once

#if defined(__x86_64__) || defined(__amd64__)
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
#endif

#include <chrono>
#include <cinttypes>
#include <utility>

#include "trpc/util/likely.h"
#include "trpc/util/chrono/chrono.h"

namespace trpc {

/// @private
namespace detail {

extern const std::chrono::nanoseconds kNanosecondsPerUnit;
#if defined(__aarch64__)
// On AArch64 we determine system timer's frequency via `cntfrq_el0`, this
// "unit" is merely used when mapping timer counter to wall clock.
constexpr std::uint32_t kUnit = 1 * 1024 * 1024;
#else
constexpr std::uint32_t kUnit = 4 * 1024 * 1024;  // ~2ms on 2GHz.
#endif

// Returns: Steady-clock, TSC.
std::pair<std::chrono::steady_clock::time_point, std::uint64_t> ReadConsistentTimestamps();

}  // namespace detail

/// @brief read value of Tsc and return to caller
/// @note TSC is almost guaranteed not to synchronize among all cores (since
///       you're likely running on a multi-chip system.). If you need such a timestamp,
///       use `std::steady_clock` instead. TSC is suitable for situations where
///       accuracy can be trade for speed.
#if defined(__x86_64__) || defined(__MACHINEX86_X64)
inline std::uint64_t ReadTsc() {
  unsigned int lo = 0;
  unsigned int hi = 0;
  asm volatile("rdtsc" : "=a" (lo), "=d" (hi));
  return ((uint64_t)hi << 32) | lo;
}
#elif defined(__aarch64__)
inline std::uint64_t ReadTsc() {
  // Sub-100MHz resolution (exactly 100MHz in our environment), not as accurate
  // as x86-64 though.
  std::uint64_t result;
  asm volatile("mrs %0, cntvct_el0" : "=r"(result));
  return result;
}
#elif defined(__powerpc__)
inline std::uint64_t ReadTsc() { return __builtin_ppc_get_timebase(); }
#else
#error Unsupported architecture.
#endif

/// @brief Converts difference between two TSCs into `std::duration<...>`.
/// @param start the begin of TSCs
/// @param to the end of TSCs
/// @return nanoseconds
inline std::chrono::nanoseconds DurationFromTsc(std::uint64_t start, std::uint64_t to) {
  // If you want to change the condition below, check comments in
  // `TimestampFromTsc` first.
  if (TRPC_UNLIKELY(start >= to)) {
    return {};
  }
  // Caution: Possible overflow here.
  return (to - start) * detail::kNanosecondsPerUnit / detail::kUnit;
}

inline std::chrono::steady_clock::time_point TimestampFromTsc(std::uint64_t tsc) {
  // TSC is not guaranteed to be consistent between NUMA domains.
  //
  // Here we use different base timestamp for different threads, so long as
  // threads are not migrated between NUMA domains, we're fine. (We'll be in a
  // little trouble if cross-NUMA migration do occur, but it's addressed below).
  thread_local std::pair<std::chrono::steady_clock::time_point, std::uint64_t> kFutureTimestamp;  // NOLINT

  // **EXACTLY** the same condition as the one tested in `DurationFromTsc`. This
  // allows the compiler to optimize the two `if`s into the same one in fast
  // path.
  //
  // What's more, this also resolves the follow issue:
  //
  // - This removes the conditional initialization of `Start`, as it can be
  //   staticially initialized at thread creation now.
  // - We move `Start.second` forward a bit each round, so `Start` is updated
  //   periodically. This compensates the inaccuracy in `DurationFromTsc` (even
  //   if it provides high resolution, it doesn't provide very good accuracy.).
  //   This also addresses TSC inconsistency between NUMA nodes.
  //
  // And all of these come with no price: the `if` in `DurationFromTsc` is
  // always needed, we just "replaced" that `if` with our own. So in fast path,
  // it's exactly the same code get executed.
  if (TRPC_UNLIKELY(tsc >= kFutureTimestamp.second)) {
    kFutureTimestamp = detail::ReadConsistentTimestamps();
  }
  return kFutureTimestamp.first - DurationFromTsc(tsc, kFutureTimestamp.second);
}

}  // namespace trpc
