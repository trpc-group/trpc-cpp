//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/runnable_entity.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include "trpc/util/internal/casting.h"

namespace trpc::fiber::detail {

// Base class for all runnable entities (those recognized by `RunQueue`).
//
// Use `isa<T>` or `dyn_cast<T>` to test its type and convert it to its
// subclass.
//
// `SchedulingGroup` is responsible for converting this object to `FiberEntity`,
// regardless of its current type.
//
// Indeed we can make things more flexible by introducing `virtual void Run()`
// to this base class. That way we can even accept callbacks and behaves like a
// thread pool if necessary. However, I don't see the need for that flexibility
// to warrant its cost.
struct RunnableEntity : ::trpc::internal::ExactMatchCastable {};

}  // namespace trpc::fiber::detail
