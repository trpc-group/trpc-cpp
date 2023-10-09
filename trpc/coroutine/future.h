//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/future.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <optional>

#include "trpc/common/future/future.h"
#include "trpc/runtime/threadmodel/fiber/detail/waitable.h"

namespace trpc::fiber {

/// @brief Wait for the future to return a result or exeception in fiber runtime, it won't block the current pthread.
/// @note  It's only used in fiber runtime.
template <class... Ts>
Future<Ts...> BlockingGet(Future<Ts...>&& f) {
  detail::Event ev;
  Future<Ts...> future;

  // Once the `future` is satisfied, our continuation will move the
  // result into callback funciton and to execute it.
  std::move(f).Then([&](Future<Ts...>&& fut) {
    future = std::move(fut);
    ev.Set();
    return MakeReadyFuture<>();
  });
  // Block until our continuation wakes us up.
  ev.Wait();
  return future;
}

template <class... Ts>
auto BlockingGet(Future<Ts...>* f) {
  return fiber::BlockingGet(std::move(*f));
}

/// @brief Same as `BlockingGet` but this one support timeout.
template <class... Ts>
std::optional<Future<Ts...>> BlockingTryGet(Future<Ts...>&& future, uint32_t timeout_ms) {
  struct State {
    detail::OneshotTimedEvent event;

    // Protects `receiver`.
    //
    // Unlike `BlockingGet`, here it's possible that after `event.Wait()` times
    // out, concurrently the future is satisfied. In this case the continuation
    // of the future will be racing with us on `receiver`.
    std::mutex lock;
    std::optional<Future<Ts...>> receiver;

    explicit State(std::chrono::steady_clock::time_point expires_at)
        : event(expires_at) {}
  };
  auto state = std::make_shared<State>(ReadSteadyClock() + std::chrono::milliseconds(timeout_ms));

  // `state` must be copied here, in case of timeout, we'll leave the scope
  // before continuation is fired.
  std::move(future).Then([state](Future<Ts...>&& fut) {
    std::scoped_lock _(state->lock);
    state->receiver.emplace(std::move(fut));
    state->event.Set();
    return MakeReadyFuture<>();
  });

  state->event.Wait();

  std::scoped_lock _(state->lock);
  return state->receiver ? std::move(state->receiver) : std::nullopt;
}

}  // namespace trpc::fiber
