//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/testing.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

// Utility for UT use. Do NOT use it in production code.

#pragma once

#include <atomic>
#include <thread>
#include <utility>

#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/testing/fiber_runtime.h"

namespace trpc::fiber::testing {

// This is for compatibility. It is recommended that users directly use trpc::testing::RunAsFiber.
template <class F>
void RunAsFiber(F&& f) {
  trpc::testing::RunAsFiber<F>(std::forward<F>(f));
}

template <class F>
void StartFiberEntityInGroup(detail::SchedulingGroup* sg, F&& f) {
  auto desc = detail::NewFiberDesc();
  desc->start_proc = std::move(f);
  desc->scheduling_group_local = false;
  sg->StartFiber(desc);
}

}  // namespace trpc::fiber::testing
