//
//
// Copyright (C) 2020 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/tsc.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/chrono/tsc.h"

#include <algorithm>
#include <iterator>

using namespace std::literals;

namespace trpc::detail {

#ifdef __aarch64__
const std::chrono::nanoseconds kNanosecondsPerUnit = [] {
  std::uint64_t freq;
  asm volatile("mrs %0, cntfrq_el0" : "=r"(freq));
  return (kUnit * (1s / 1ns) / freq) * 1ns;
}();
#else
const std::chrono::nanoseconds kNanosecondsPerUnit = [] {
  int retries = 0;

  while (true) {
    // We try to determine the result multiple times, and filter out the
    // outliers from the result, so as to be as accurate as possible.
    constexpr auto kTries = 64;
    std::chrono::nanoseconds elapsed[kTries];

    for (int i = 0; i != kTries; ++i) {
      auto tsc0 = ReadTsc();
      auto start = ReadSteadyClock();
      while (ReadTsc() - tsc0 < kUnit) {
        // NOTHING.
      }
      elapsed[i] = ReadSteadyClock() - start;
    }
    std::sort(std::begin(elapsed), std::end(elapsed));

    // Only results in the middle are used.
    auto since = kTries / 3, upto = kTries / 3 * 2;
    if (elapsed[upto] - elapsed[since] > 1us) {  // I think it's big enough.
      if (++retries > 5) {
        // RAW_LOG(WARNING,
        //         "We keep failing in determining TSC rate. Busy system?");
      }
      continue;
    }

    // The average of the results in the middle is the final result.
    std::chrono::nanoseconds sum{};
    for (int i = since; i != upto; ++i) {
      sum += elapsed[i];
    }
    return sum / (upto - since);
  }
}();
#endif

std::pair<std::chrono::steady_clock::time_point, std::uint64_t> ReadConsistentTimestamps() {
  while (true) {
    // Wall clock is read twice to detect preemption by other threads. We need
    // wall clock and TSC to be close enough to be useful.
    auto s1 = ReadSteadyClock();
    auto tsc = ReadTsc();
    auto s2 = ReadSteadyClock();

    // I'm not aware of any system on which `clock_gettime` would consistently
    // consume more than 200ns.
    if (s2 - s1 > std::chrono::microseconds(1)) {
      continue;  // We were preempted during execution, `tsc` and `s1` are
                 // likely to be incoherent. Try again then.
    }

    return std::pair(s1 + (s2 - s1) / 2 + detail::kNanosecondsPerUnit, tsc + detail::kUnit);
  }
}

}  // namespace trpc::detail
