//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
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

namespace trpc::object_pool {

#define TRPC_CHECK_CLASS_MEMBER_HELPER(name)                                                \
  template <typename T>                                                                     \
  class Has##name {                                                                         \
   private:                                                                                 \
    template <typename U>                                                                   \
    constexpr static auto Check(int) -> decltype(std::declval<U>().name, std::true_type()); \
    template <typename U>                                                                   \
    constexpr static std::false_type Check(...);                                            \
                                                                                            \
   public:                                                                                  \
    enum { value = std::is_same<decltype(Check<T>(0)), std::true_type>::value };            \
    Has##name() = delete;                                                                   \
    ~Has##name() = delete;                                                                  \
  }

#define TRPC_HAS_CLASS_MEMBER(cls, name) Has##name<cls>::value

/// @brief Used to set the type of object pool used for a specific type and the allocation/release parameters.
template <class T>
struct ObjectPoolTraits {
  // Set the object pool type used for type T.
  // static constexpr ObjectPoolType kType = ...;

  // Maximum number of objects.
  // static constexpr size_t kMaxObjectNum = ...;

  static_assert(sizeof(T) == 0,
                "You need to specialize `trpc::object_pool::ObjectPoolTraits` to "
                "specify parameters before using `object_pool::Xxx`.");
};

// Check if the object pool extraction class has a member named 'kMaxObjectNum'.
TRPC_CHECK_CLASS_MEMBER_HELPER(kMaxObjectNum);

}  // namespace trpc::object_pool
