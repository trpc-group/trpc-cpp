//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/fiber_local.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include "trpc/runtime/threadmodel/fiber/detail/fiber_entity.h"
#include "trpc/util/internal/index_alloc.h"

namespace trpc {

struct FiberLocalIndexTag;
struct TrivialFiberLocalIndexTag;

/// @brief `T` needs to be `DefaultConstructible`.
///        You should normally use this class as static / member variable.
///        In case of variable in stack, just use automatic variable (stack variable) instead.
/// @note  It only uses in fiber runtime.
template <class T>
class FiberLocal {
  inline static constexpr auto is_using_trivial_fls_v =
      std::is_trivial_v<T> && sizeof(T) <= sizeof(fiber::detail::FiberEntity::trivial_fls_t);

 public:
  FiberLocal() : slot_index_(GetIndexAlloc()->Next()) {}

  ~FiberLocal() { GetIndexAlloc()->Free(slot_index_); }

  T* operator->() const noexcept { return Get(); }
  T& operator*() const noexcept { return *Get(); }

  T* Get() const noexcept {
    auto current_fiber = fiber::detail::GetCurrentFiberEntity();
    if constexpr (is_using_trivial_fls_v) {
      return reinterpret_cast<T*>(current_fiber->GetTrivialFls(slot_index_));
    } else {
      auto ptr = current_fiber->GetFls(slot_index_);
      if (!*ptr) {
        *ptr = MakeErased<T>();
      }
      return static_cast<T*>(ptr->Get());
    }
  }

  /// @brief GetIndexAlloc.
  /// @note  Only used inside the framework.
  static internal::IndexAlloc* GetIndexAlloc() {
    if constexpr (is_using_trivial_fls_v) {
      return internal::IndexAlloc::For<TrivialFiberLocalIndexTag>();
    } else {
      return internal::IndexAlloc::For<FiberLocalIndexTag>();
    }
  }

 private:
  std::size_t slot_index_;
};

}  // namespace trpc
