//
//
// Copyright (C) 2020 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/hazptr/entry.h.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <atomic>

namespace trpc {

class HazptrDomain;

}  // namespace trpc

namespace trpc::hazptr {

class Object;

/// @brief Everything related to the `HazptrDomain` (control structure) are defined here.
struct Entry {
  std::atomic<const Object*> ptr;
  std::atomic<bool> active{false};
  HazptrDomain* domain;
  Entry* next{nullptr};

  /// @brief Try to acquire entry
  /// @return bool true: ok; false: failed
  bool TryAcquire() noexcept {
    return !active.load(std::memory_order_relaxed) && !active.exchange(true, std::memory_order_relaxed);
  }

  /// @brief Release entry
  void Release() noexcept { active.store(false, std::memory_order_relaxed); }

  /// @brief Get pointer of `Object` of this entry
  /// @return Object*
  const Object* TryGetPtr() noexcept { return ptr.load(std::memory_order_acquire); }

  /// @brief Save pointer of `Object` to this entry
  /// @param ptr Object*
  void ExposePtr(const Object* ptr) noexcept { this->ptr.store(ptr, std::memory_order_release); }
};

}  // namespace trpc::hazptr
