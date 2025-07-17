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

#include "trpc/util/function.h"

namespace trpc {

// Simulate creating a fiber and return false, for testing
bool StartFiberDetached(Function<void()>&& start_proc) { return false; }

// Simulate creating a fiber and return false, for testing
bool StartFiberDetached(Fiber::Attributes&& attrs, Function<void()>&& start_proc) {
  return false;
}

}  // namespace trpc
