//
//
// Copyright (C) 2021 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/fiber_desc.h.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include "trpc/runtime/threadmodel/fiber/detail/runnable_entity.h"
#include "trpc/util/align.h"
#include "trpc/util/function.h"
#include "trpc/util/object_pool/object_pool_ptr.h"

namespace trpc::fiber::detail {

class ExitBarrier;

// This structure stores information describing how to instantiate a
// `FiberEntity`. The instantiation is deferred to first run of the fiber.
//
// This approach should help performance since:
//
// - Reduced memory footprint: We don't need to allocate a stack until actual
//   run.
//
// - Alleviated producer-consumer effect: The fiber stack is allocated in fiber
//   worker, where most (exited) fiber' stack is freed. This promotes more
//   thread-local-level reuse. If we keep allocating stack from thread X and
//   consume it in thread Y, we'd have a hard time in transfering fiber stack
//   between them (mostly because we can't afford a big transfer-size to avoid
//   excessive memory footprint.).
struct alignas(hardware_destructive_interference_size) FiberDesc
    : RunnableEntity {
  Function<void()> start_proc;
  trpc::object_pool::LwSharedPtr<trpc::fiber::detail::ExitBarrier> exit_barrier;
  std::uint64_t last_ready_tsc;
  bool scheduling_group_local;
  bool is_fiber_reactor = false;

  FiberDesc();
};

// Creates a new fiber startup descriptor.
FiberDesc* NewFiberDesc() noexcept;

// Destroys a fiber startup descriptor.
//
// In most cases this method is called by `InstantiateFiberEntity`. Calling this
// method yourself is almost always an error.
void DestroyFiberDesc(FiberDesc* desc) noexcept;

}  // namespace trpc::fiber::detail
