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

#include <type_traits>
#include <utility>

#include "trpc/util/object_pool/disabled.h"
#include "trpc/util/object_pool/global.h"
#include "trpc/util/object_pool/shared_nothing.h"
#include "trpc/util/object_pool/util.h"

namespace trpc::object_pool {

/// @brief The type of object pool been used.
enum class ObjectPoolType {
  /// Not using an object pool.
  kDisabled,

  /// Using a shared-nothing object pool.
  /// Behavior: objects allocated from specific thread will also be returned to that same thread during deallocation.
  kSharedNothing,

  /// Using a global object pool. It's the default object pools used by framework.
  /// Behavior: The allocation and deallocation of objects can span across thread's local pool.
  kGlobal
};

/// @brief The internal encapsulated implementation of object_pool. Do not use the interfaces of this namespace
///        directly.
/// @private For internal use purpose only.
namespace detail {

template <class T>
inline T* New() {
  constexpr auto kType = ObjectPoolTraits<T>::kType;

  if constexpr (kType == ObjectPoolType::kDisabled) {
    return trpc::object_pool::disabled::New<T>();
  } else if constexpr (kType == ObjectPoolType::kSharedNothing) {
    return trpc::object_pool::shared_nothing::New<T>();
  } else if constexpr (kType == ObjectPoolType::kGlobal) {
    return trpc::object_pool::global::New<T>();
  } else {
    static_assert(sizeof(T) == 0, "Unexpected object pool type.");
  }
}

template <class T>
inline void Delete(T* ptr) {
  constexpr auto kType = ObjectPoolTraits<T>::kType;

  if constexpr (kType == ObjectPoolType::kDisabled) {
    trpc::object_pool::disabled::Delete<T>(ptr);
  } else if constexpr (kType == ObjectPoolType::kSharedNothing) {
    shared_nothing::Delete<T>(ptr);
  } else if constexpr (kType == ObjectPoolType::kGlobal) {
    trpc::object_pool::global::Delete<T>(ptr);
  } else {
    static_assert(sizeof(T) == 0, "Unexpected object pool type.");
  }
}

}  // namespace detail

/// @brief Allocate and construct an object (thread-safe).
/// @code
/// example
///   struct A {
///     int a;
///   };
///   template <>
///   struct ObjectPoolTraits<A> {
///   #if defined(TRPC_DISABLED_OBJECTPOOL)
///     static constexpr auto kType = ObjectPoolType::kDisabled;
///   #elif defined(TRPC_SHARED_NOTHING_OBJECTPOOL)
///     static constexpr auto kType = ObjectPoolType::kSharedNothing;
///   #else
///     static constexpr auto kType = ObjectPoolType::kGlobal;
///   #endif
///   A* a_p = trpc::object_pool::New<A>();
///   trpc::object_pool::Delete(a_p);
/// @endcode
template <class T, class... Args>
T* New(Args&&... args) {
  // Remove CV attributes. We do not differentiate between T, const T, volatile T, and const volatile T,
  // they are all treated as T.
  auto* p = static_cast<T*>(detail::New<std::remove_cv_t<T>>());
  if (p) {
    new (p) T(std::forward<Args>(args)...);
    return p;
  }

  return nullptr;
}

/// @brief Delete and release an object, then recycle the memory back to the object pool after executing the destructor
///        (thread-safe).
template <class T>
void Delete(T* p) {
  if (p) {
    p->~T();
    detail::Delete(const_cast<std::remove_cv_t<T>*>(p));
  }
}

}  // namespace trpc::object_pool
