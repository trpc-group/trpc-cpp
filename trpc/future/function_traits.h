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

#include <functional>
#include <tuple>

namespace trpc {

/// @brief Declaration for only one template argument.
template <typename T>
struct function_traits;

/// @brief Common function.
template <typename Ret, typename... Args>
struct function_traits<Ret(Args...)> {
 public:
  static constexpr size_t arity = sizeof...(Args);

  using tuple_type = std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...>;
};

/// @brief Function pointer.
template <typename Ret, typename... Args>
struct function_traits<Ret (*)(Args...)> : public function_traits<Ret(Args...)> {};

/// @brief Member function.
template <typename Ret, typename Class, typename... Args>
struct function_traits<Ret (Class::*)(Args...)> : public function_traits<Ret(Args...)> {};

/// @brief Const member function.
template <typename Ret, typename Class, typename... Args>
struct function_traits<Ret (Class::*)(Args...) const> : public function_traits<Ret(Args...)> {};

/// @brief Standard function.
template <typename Ret, typename... Args>
struct function_traits<std::function<Ret(Args...)>> : public function_traits<Ret(Args...)> {};

// Function object.
template <typename Callable>
struct function_traits : public function_traits<decltype(&Callable::operator())> {};

}  // namespace trpc
