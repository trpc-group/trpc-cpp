//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/async.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <type_traits>
#include <utility>

#include "trpc/common/future/basics.h"
#include "trpc/common/future/future.h"
#include "trpc/coroutine/fiber_execution_context.h"
#include "trpc/coroutine/fiber.h"

namespace trpc::fiber {

/// @brief Runs `f` with `args...` asynchronously.
/// @note  Note that this method is only available in fiber runtime.
///        If you want to start a fiber from pthread, use `StartFiberDetached` instead.
template <class F, class... Args,
          class R = trpc::futurize_t<std::invoke_result_t<F&&, Args&&...>>>
R Async(Launch policy, std::size_t scheduling_group,
        FiberExecutionContext* execution_context, F&& f, Args&&... args) {
  TRPC_CHECK(policy == Launch::Post || policy == Launch::Dispatch);
  trpc::as_promise_t<R> p;
  auto rc = p.GetFuture();
  auto proc = [p = std::move(p), f = std::forward<F>(f),
               args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
    if constexpr (std::is_same_v<Future<>, R>) {
      std::apply(f, std::move(args));
      p.SetValue();
    } else {
      p.SetValue(std::apply(f, std::move(args)));
    }
  };

  trpc::StartFiberDetached(
      Fiber::Attributes{.launch_policy = policy,
                        .scheduling_group = scheduling_group,
                        .execution_context = execution_context},
      std::move(proc));
  return rc;
}

template <class F, class... Args,
          class R = trpc::futurize_t<std::invoke_result_t<F&&, Args&&...>>>
R Async(Launch policy, std::size_t scheduling_group, F&& f, Args&&... args) {
  return Async(policy, scheduling_group, trpc::FiberExecutionContext::Current(),
               std::forward<F>(f), std::forward<Args>(args)...);
}

template <class F, class... Args,
          class R = trpc::futurize_t<std::invoke_result_t<F&&, Args&&...>>>
R Async(Launch policy, F&& f, Args&&... args) {
  return Async(policy, Fiber::kNearestSchedulingGroup, std::forward<F>(f),
               std::forward<Args>(args)...);
}

template <class F, class... Args,
          class = std::enable_if_t<std::is_invocable_v<F&&, Args&&...>>>
auto Async(F&& f, Args&&... args) {
  return Async(Launch::Post, std::forward<F>(f), std::forward<Args>(args)...);
}

}  // namespace trpc::fiber
