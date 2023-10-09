//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/internal/index_alloc.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <cstddef>
#include <mutex>
#include <vector>

#include "trpc/util/internal/never_destroyed.h"

namespace trpc::internal {

/// @brief This class helps you allocate indices. Indices are numbered from 0.
/// @note that this class does NOT perform well. It's not intended for use in
///       scenarios where efficient index allocation is needed.
class IndexAlloc {
 public:
  /// @brief To prevent interference between index allocation for different purpose, you
  ///        can use tag type to separate different allocations.
  /// @return Return a pointer to type `IndexAlloc`.
  template <class Tag>
  static IndexAlloc* For() {
    static NeverDestroyedSingleton<IndexAlloc> ia;
    return ia.Get();
  }

  /// @brief Get next available index. If there's previously freed index (via `Free`),
  ///       it's returned, otherwise a new one is allocated.
  /// @return next id
  std::size_t Next();

  ///@brief Free an index. It can be reused later.
  void Free(std::size_t index);

 private:
  friend class NeverDestroyedSingleton<IndexAlloc>;
  // You shouldn't instantiate this class yourself, call `For<...>()` instead.
  IndexAlloc() = default;
  ~IndexAlloc() = default;

  std::mutex lock_;
  std::size_t current_{};
  std::vector<std::size_t> recycled_;
};

}  // namespace trpc::internal
