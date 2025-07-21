//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/internal/meta.h.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <type_traits>
#include <utility>

namespace trpc::internal {

// Some meta-programming helpers.

template <class F>
constexpr auto is_valid(F&& f) {
  return [f = std::forward<F>(f)](auto&&... args) constexpr {
    // FIXME: Perfect forwarding.
    return std::is_invocable_v<F, decltype(args)...>;
  };
}

/// @brief `x` should be used as placeholder's name in` expr`.
#define TRPC_INTERNAL_IS_VALID(expr) ::trpc::internal::is_valid([](auto&& x) -> decltype(expr) {})

/// @brief Clang's (as of Clang 10) `std::void_t` suffers from CWG 1558, to be able to
///       use `std::void_t` in SFINAE, we roll our own one that immune to this issue.
/// @sa: https://bugs.llvm.org/show_bug.cgi?id=33655
namespace detail {

template <class...>
struct make_void {
  using type = void;
};

}  // namespace detail

template <class... Ts>
using void_t = typename detail::make_void<Ts...>::type;

}  // namespace trpc::internal
