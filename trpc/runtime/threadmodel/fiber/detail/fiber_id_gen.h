//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/id_alloc.h.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <atomic>

#include "trpc/util/likely.h"

namespace trpc::fiber::detail {

template <class Traits>
class AllocImpl;

/// @brief This method differs from `IndexAlloc` in that `IndexAlloc` tries its best to
///        reuse indices, in trade of performance. In contrast, this method does not
///        try to reuse ID, for better performance.
/// @example `Traits` is defined as:
///           struct Traits {
///             /* (Integral) type of ID. */
///             using Type = ...;
///
///             /* Minimum / maximum value of ID. The interval is left-closed, right-open.
///                Technically this means we can waste one ID if the entire value range of
///               `type` can be used, however, wasting it simplifies our implementation,
///                and shouldn't hurt much anyway.
///             */
///             static constexpr auto kMin = ...;
///             static constexpr auto kMax = ...;
///
///             /* For better performance, the implementation grabs a batch of IDs from
///                global counter to its thread local cache. This parameter controls batch size.
///             */
///             static constexpr auto kBatchSize = ...;
///           };
template <class Traits>
inline auto Next() {
  return AllocImpl<Traits>::Next();
}

template <class Traits>
class AllocImpl {
 public:
  using Type = typename Traits::Type;

  /// @brief This method will likely be inlined.
  static Type Next() {
    // Let's see if our thread local cache can serve us.
    if (auto v = current_++; TRPC_LIKELY(v < max_)) {
      return v;
    }

    return SlowNext();
  }

 private:
  static Type SlowNext();

 private:
  static inline std::atomic<Type> global_{Traits::kMin};
  static inline thread_local Type current_ = 0;  // NOLINT
  static inline thread_local Type max_ = 0;      // NOLINT
};

template <class Traits>
typename AllocImpl<Traits>::Type AllocImpl<Traits>::SlowNext() {
  // Get more IDs from global counter...
  Type read, next;
  do {
    read = global_.load(std::memory_order_relaxed);
    next = read + Traits::kBatchSize;
    current_ = read;
    max_ = next;
    if (next >= Traits::kMax) {
      max_ = Traits::kMax;
      next = Traits::kMin;
    }
  } while (!global_.compare_exchange_weak(read, next, std::memory_order_relaxed));

  // ... and retry.
  return Next();
}

}  // namespace trpc::fiber::detail
