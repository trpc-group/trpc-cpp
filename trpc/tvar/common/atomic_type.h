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

#pragma once

#include <atomic>
#include <mutex>
#include <type_traits>

#include "trpc/tvar/common/op_util.h"

namespace trpc::tvar {

/// @brief Generic implementation of atomic for user class.
/// @tparam T Value type.
/// @tparam Enabler Designed for template specialization.
/// @private
template <typename T, typename Enabler = void>
class AtomicType {
  template <typename>
  friend class GlobalValue;

 public:
  /// @brief Thread-safe to read.
  T Load() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return value_;
  }

  /// @brief Thread-safe to write.
  void Store(T const& new_value) {
    std::unique_lock<std::mutex> lock(mutex_);
    value_ = new_value;
  }

  /// @brief Thread-safe to write, and return previous value.
  void Exchange(T* prev, T const& new_value) {
    std::unique_lock<std::mutex> lock(mutex_);
    *prev = value_;
    value_ = new_value;
  }

  /// @brief Thread-safe to write, but use user defined operator.
  template <typename Op, typename T1>
  void Modify(T1 t1) {
    std::unique_lock<std::mutex> lock(mutex_);
    Op()(&value_, t1);
  }

  /// @brief Merge value into global.
  template <typename Op, typename GlobalValue, typename T1>
  void MergeGlobal(GlobalValue* global_value, T1 t1) {
    std::unique_lock<std::mutex> lock(mutex_);
    Op()(global_value, &value_, t1);
  }

 private:
  // Protected value.
  T value_{T()};

  // Lock to protect value.
  mutable std::mutex mutex_;
};

/// @brief Specialized implementation for integral or floating type.
/// @private
template <typename T>
class AtomicType<T, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, void>> {
 public:
  /// @brief Thread-safe to read.
  T Load() const { return value_.load(std::memory_order_relaxed); }

  /// @brief Thread-safe to write.
  void Store(T new_value) { value_.store(new_value, std::memory_order_relaxed); }

  /// @brief Thread-safe to write, and get previous value.
  void Exchange(T* prev, T new_value) {
    *prev = value_.exchange(new_value, std::memory_order_relaxed);
  }

  /// @brief Use compare_exchange_weak.
  bool CompareExchangeWeak(T* expected, T new_value) {
    return value_.compare_exchange_weak(*expected, new_value, std::memory_order_relaxed);
  }

  /// @brief Higher performance for OpAdd on the assumption that only one thread may write at the same time.
  template <typename Op, typename T1>
  void Modify(T1 t1) {
    // Maybe multiple threads write at the same time.
    if constexpr (!std::is_same_v<Op, OpAdd<T>>) {
      auto old_value = Load();
      auto new_value = old_value;
      Op()(&new_value, t1);
      while (!value_.compare_exchange_weak(old_value, new_value, std::memory_order_relaxed)) {
        new_value = old_value;
        Op()(&new_value, t1);
      }
    // Make sure only one thread may write at the same time, eg. Gauge and Counter.
    } else {
      auto new_value = Load();
      Op()(&new_value, t1);
      Store(new_value);
    }
  }

 private:
  // Use standard atomic to make thread-safe.
  std::atomic<T> value_{T()};
};

}  // namespace trpc::tvar
