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

#include <tuple>
#include <type_traits>

#include "trpc/future/types.h"

namespace trpc {

/// @brief Helper for is_rvalue_reference by multiple types.
template <class... T>
constexpr auto are_rvalue_refs_v = std::conjunction_v<std::is_rvalue_reference<T>...>;
static_assert(are_rvalue_refs_v<int&&, char&&>);
static_assert(!are_rvalue_refs_v<int, char&&>);

/// @brief Declaration first.
template <class TT, template <class...> class UT>
struct rebind;

/// @brief Convert Rebind_t<Future<int, double>, Promise<>> to Promise<int, double>.
template <template <class...> class TT, class... T, template <class...> class UT>
struct rebind<TT<T...>, UT> {
  using type = UT<T...>;
};

/// @brief Helper for rebind.
template <class TT, template <class...> class UT>
using rebind_t = typename rebind<TT, UT>::type;
static_assert(std::is_same_v<rebind_t<std::common_type<>, Types>, Types<>>);
static_assert(std::is_same_v<rebind_t<std::common_type<int, char>, Types>, Types<int, char>>);

/// @brief Declaration for Future.
template <class... T>
class Future;

/// @brief Declaration for Promise.
template <class... T>
class Promise;

/// @brief Declaration for Boxed.
template <class... T>
class Boxed;

/// @brief Futurize a type.
template <class... T>
struct futurize {
  using type = Future<T...>;
};

/// @brief Specilization for void.
template <>
struct futurize<void> : futurize<> {};

/// @brief Specilization for skipping Future prefix.
template <class... T>
struct futurize<Future<T...>> : futurize<T...> {};

/// @brief Helper for futurize.
template <class... T>
using futurize_t = typename futurize<T...>::type;
static_assert(std::is_same_v<futurize_t<int>, Future<int>>);
static_assert(std::is_same_v<futurize_t<Future<>>, Future<>>);
static_assert(std::is_same_v<futurize_t<Future<int>>, Future<int>>);

/// @brief Concatenate T... in multiple Future<T...>.
///        For example, convert flatten<Future<T1, T2>, Future<T3>> to Future<T1, T2, T3>.
template <class... T>
struct flatten {
  using type = rebind_t<types_cat_t<rebind_t<T, Types>...>, Future>;
};

/// @brief Helper for flatten.
template <class... T>
using flatten_t = typename flatten<T...>::type;
static_assert(std::is_same_v<flatten_t<Future<void*>, Future<>, Future<char>>, Future<void*, char>>);
static_assert(std::is_same_v<flatten_t<Future<>, Future<>>, Future<>>);

/// @brief For rebinding Future<T...>.
template <class T>
using as_boxed_t = rebind_t<T, Boxed>;

/// @brief Helper for rebind Promise.
template <class T>
using as_promise_t = rebind_t<T, Promise>;
static_assert(std::is_same_v<as_boxed_t<Future<>>, Boxed<>>);
static_assert(std::is_same_v<as_boxed_t<Future<int, char>>, Boxed<int, char>>);
static_assert(std::is_same_v<as_promise_t<Future<>>, Promise<>>);
static_assert(std::is_same_v<as_promise_t<Future<int, char>>, Promise<int, char>>);

/// @brief Value type we will get from Boxed<...>::Get.
///        Depending on what's in T..., unboxed_type_t<T...> can be:
///        - void if T... is empty;
///        - R is there is only one type R in T...;
///        - std::tuple<T...> otherwise.
template <class... T>
using unboxed_type_t = typename std::conditional_t<
    sizeof...(T) == 0, std::common_type<void>,
    std::conditional_t<sizeof...(T) == 1, std::common_type<T...>,
                       std::common_type<std::tuple<T...>>>>::type;

}  // namespace trpc
