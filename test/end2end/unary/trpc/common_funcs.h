// Copyright (c) 2023, Tencent Inc.
// All rights reserved.

#pragma once

#include "trpc/coroutine/fiber.h"
#include "trpc/runtime/runtime.h"

namespace trpc::testing {

void SleepFor(uint32_t ms) {
  if (::trpc::runtime::IsInFiberRuntime()) {
    ::trpc::FiberSleepFor(std::chrono::milliseconds(ms));
  } else {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  }
}

}  // namespace trpc::testing
