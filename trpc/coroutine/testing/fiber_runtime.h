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

#include <utility>

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber/runtime.h"
#include "trpc/runtime/runtime.h"
#include "trpc/util/thread/latch.h"

namespace trpc::testing {

/// @brief Run a task as fiber.
/// @tparam F Any function type
/// @param f task logic
template <class F>
void RunAsFiber(F&& f) {
  trpc::fiber::StartRuntime();
  trpc::runtime::SetRuntimeType(trpc::runtime::kFiberRuntime);
  trpc::Latch l(1);
  trpc::Fiber([&, f = std::forward<F>(f)] {
    f();
    l.count_down();
  }).Detach();
  l.wait();
  trpc::fiber::TerminateRuntime();
}

}  // namespace trpc::testing

// for compatible, use `trpc::testing::RunAsFiber` instead.
template <class F>
void RunAsFiber(F&& f) {
  trpc::testing::RunAsFiber<F>(std::forward<F>(f));
}
