//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/fiber.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/coroutine/fiber.h"

#include <thread>
#include <utility>
#include <vector>

#include "trpc/coroutine/fiber_execution_context.h"
#include "trpc/coroutine/fiber/runtime.h"
#include "trpc/runtime/threadmodel/fiber/detail/fiber_entity.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling_group.h"
#include "trpc/runtime/threadmodel/fiber/detail/waitable.h"
#include "trpc/util/check.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/random.h"

namespace trpc {

namespace {

fiber::detail::SchedulingGroup* GetSchedulingGroup(std::size_t id) {
  if (TRPC_LIKELY(id == Fiber::kNearestSchedulingGroup)) {
    return fiber::detail::NearestSchedulingGroup();
  } else if (id == Fiber::kUnspecifiedSchedulingGroup) {
    return fiber::detail::GetSchedulingGroup(
        Random<std::size_t>(0, fiber::GetSchedulingGroupCount() - 1));
  } else {
    return fiber::detail::GetSchedulingGroup(id);
  }
}

}  // namespace

Fiber::Fiber() = default;

Fiber::~Fiber() {
  TRPC_CHECK(!Joinable(),
             "You need to call either `join()` or `detach()` prior to destroy "
             "a fiber.");
}

Fiber::Fiber(const Attributes& attr, Function<void()>&& start) {
  auto sg = GetSchedulingGroup(attr.scheduling_group);
  TRPC_CHECK(sg, "No scheduling group is available?");

  if (attr.execution_context) {
    start = [start = std::move(start), ec = object_pool::LwSharedPtr(object_pool::lw_shared_ref_ptr,
                                                                     attr.execution_context)] {
      ec->Execute(start);
    };
  }

  // `desc` will cease to exist once `start` returns. We don't own it.
  auto desc = fiber::detail::NewFiberDesc();
  desc->start_proc = std::move(start);
  desc->scheduling_group_local = attr.scheduling_group_local;
  desc->is_fiber_reactor = attr.is_fiber_reactor;

  // If `join()` is called, we'll sleep on this.
  desc->exit_barrier = object_pool::MakeLwShared<fiber::detail::ExitBarrier>();
  join_impl_ = desc->exit_barrier;

  // Schedule the fiber.
  if (attr.launch_policy == fiber::Launch::Post) {
    TRPC_ASSERT(sg->StartFiber(desc));
  } else {
    sg->SwitchTo(fiber::detail::GetCurrentFiberEntity(),
                 fiber::detail::InstantiateFiberEntity(sg, desc));
  }
}

void Fiber::Detach() {
  TRPC_CHECK(Joinable(), "The fiber is in detached state.");
  join_impl_ = nullptr;
}

void Fiber::Join() {
  TRPC_CHECK(Joinable(), "The fiber is in detached state.");
  join_impl_->Wait();
  join_impl_.Reset();
}

bool Fiber::Joinable() const { return !!join_impl_; }

Fiber::Fiber(Fiber&&) noexcept = default;
Fiber& Fiber::operator=(Fiber&&) noexcept = default;

bool StartFiberDetached(Function<void()>&& start_proc) {
  auto desc = fiber::detail::NewFiberDesc();
  desc->start_proc = std::move(start_proc);
  TRPC_CHECK(!desc->exit_barrier);
  desc->scheduling_group_local = false;

  return fiber::detail::NearestSchedulingGroup()->StartFiber(desc);
}

bool StartFiberDetached(Fiber::Attributes&& attrs, Function<void()>&& start_proc) {
  auto sg = GetSchedulingGroup(attrs.scheduling_group);

  if (attrs.execution_context) {
    start_proc = [start_proc = std::move(start_proc),
                  ec = object_pool::LwSharedPtr(object_pool::lw_shared_ref_ptr,
                                                std::move(attrs.execution_context))] {
      ec->Execute(start_proc);
    };
  }

  auto desc = fiber::detail::NewFiberDesc();
  desc->start_proc = std::move(start_proc);
  TRPC_CHECK(!desc->exit_barrier);
  desc->scheduling_group_local = attrs.scheduling_group_local;

  if (attrs.launch_policy == fiber::Launch::Post) {
    return sg->StartFiber(desc);
  }

  sg->SwitchTo(fiber::detail::GetCurrentFiberEntity(),
               fiber::detail::InstantiateFiberEntity(sg, desc));

  return true;
}

bool BatchStartFiberDetached(std::vector<Function<void()>>&& start_procs) {
  std::vector<fiber::detail::FiberDesc*> descs;
  for (auto&& e : start_procs) {
    auto desc = fiber::detail::NewFiberDesc();
    desc->start_proc = std::move(e);
    TRPC_CHECK(!desc->exit_barrier);
    desc->scheduling_group_local = false;
    descs.push_back(desc);
  }

  fiber::detail::NearestSchedulingGroup()->StartFibers(
      descs.data(), descs.data() + descs.size());
  return true;
}

void FiberYield() {
  auto self = fiber::detail::GetCurrentFiberEntity();
  TRPC_CHECK(self, "this_fiber::Yield may only be called in fiber environment.");
  self->scheduling_group->Yield(self);
}

void FiberSleepUntil(const std::chrono::steady_clock::time_point& expires_at) {
  if (trpc::fiber::detail::IsFiberContextPresent()) {
    fiber::detail::WaitableTimer wt(expires_at);
    wt.wait();
    return;
  }

  auto now = ReadSteadyClock();
  if (expires_at <= now) {
    return;
  }

  std::this_thread::sleep_for(expires_at - now);
}

void FiberSleepFor(const std::chrono::nanoseconds& expires_in) {
  FiberSleepUntil(ReadSteadyClock() + expires_in);
}

bool IsRunningInFiberWorker() {
  return fiber::detail::GetCurrentFiberEntity() != nullptr;
}

std::size_t GetFiberCount() {
  return fiber::detail::GetFiberCount();
}

std::size_t GetFiberQueueSize() {
  return fiber::GetFiberQueueSize();
}

}  // namespace trpc
