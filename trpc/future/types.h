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

namespace trpc {

/// @brief Helper for types_at, types_cat, types_contains and types_erase.
template <class... T>
struct Types {};

/// @brief Get type at specified location.
template <class T, std::size_t I>
struct types_at;

/// @brief Specialization for recursive boundary.
template <class T, class... R>
struct types_at<Types<T, R...>, 0> {
  using type = T;
};

/// @brief Recursive implementation for multiple Types arguments.
template <class T, class... R, std::size_t I>
struct types_at<Types<T, R...>, I> : types_at<Types<R...>, I - 1> {};

/// @brief Helper for types_at.
template <class T, std::size_t I>
using types_at_t = typename types_at<T, I>::type;
static_assert(std::is_same_v<types_at_t<Types<int, char, void>, 1>, char>);

/// @brief Analogous to std::tuple_cat.
template <class... T>
struct types_cat;

/// @brief Specialization for zero argument.
template <>
struct types_cat<> {
  using type = Types<>;
};

/// @brief Specialization for recursive boundary, one Types.
template <class... T>
struct types_cat<Types<T...>> {
  using type = Types<T...>;
};

/// @brief Linear semantic for multiple Types.
template <class... T, class... U, class... V>
struct types_cat<Types<T...>, Types<U...>, V...> : types_cat<Types<T..., U...>, V...> {};

/// @brief Helper for types_cat.
template <class... T>
using types_cat_t = typename types_cat<T...>::type;
static_assert(std::is_same_v<types_cat_t<Types<int, double>, Types<void*>>, Types<int, double, void*>>);
static_assert(std::is_same_v<types_cat_t<Types<int, double>, Types<void*>, Types<>>, Types<int, double, void*>>);
static_assert(std::is_same_v<types_cat_t<Types<int, double>, Types<void*>, Types<unsigned>>,
              Types<int, double, void*, unsigned>>);

/// @brief Declaration first.
template <class T, class U>
struct types_contains;

/// @brief Specialization for empty Types.
template <class T>
struct types_contains<Types<>, T> : std::false_type {};

/// @brief Specialization for single argument Types.
template <class T, class U>
struct types_contains<Types<T>, U> : std::is_same<T, U> {};

/// @brief Recursive implementation for multiple argument Types.
template <class T, class U, class... R>
struct types_contains<Types<T, R...>, U> : std::conditional_t<std::is_same_v<T, U>, std::true_type,
                                           types_contains<Types<R...>, U>> {};

/// @brief Helper for types_contains.
template <class T, class U>
constexpr auto types_contains_v = types_contains<T, U>::value;
static_assert(types_contains_v<Types<int, char>, char>);
static_assert(!types_contains_v<Types<int, char>, char*>);

/// @brief Declaration first.
template <class T, class U>
struct types_erase;

/// @brief Specialization for empty Types.
template <class T>
struct types_erase<Types<>, T> {
  using type = Types<>;
};

/// @brief Recursive implementation.
template <class T, class U, class... R>
struct types_erase<Types<T, R...>, U> : types_cat<std::conditional_t<!std::is_same_v<T, U>, Types<T>, Types<>>,
                                        typename types_erase<Types<R...>, U>::type> {};

/// @brief Helper for types_erase.
template <class T, class U>
using types_erase_t = typename types_erase<T, U>::type;
static_assert(std::is_same_v<types_erase_t<Types<int, void, char>, void>, Types<int, char>>);
static_assert(std::is_same_v<types_erase_t<Types<>, void>, Types<>>);
static_assert(std::is_same_v<types_erase_t<Types<int, char>, void>, Types<int, char>>);

}  // namespace trpc
