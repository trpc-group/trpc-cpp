//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/timer.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/coroutine/fiber_timer.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <utility>

#include "trpc/coroutine/fiber_execution_context.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber/runtime.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling_group.h"

namespace trpc {

std::uint64_t SetFiberTimer(std::chrono::steady_clock::time_point at, Function<void()>&& cb) {
  return SetFiberTimer(at, [cb = std::move(cb)](auto) { cb(); });
}

std::uint64_t SetFiberTimer(std::chrono::steady_clock::time_point at,
                            Function<void(std::uint64_t)>&& cb) {
  auto ec =
      object_pool::LwSharedPtr(object_pool::lw_shared_ref_ptr, FiberExecutionContext::Current());
  Function<void(std::uint64_t)> mcb = [cb = std::move(cb), ec = std::move(ec)](auto timer_id) mutable {
    Fiber::Attributes attr;
    attr.execution_context = ec.Get();
    bool start_fiber =
        StartFiberDetached(std::move(attr), [cb = std::move(cb), timer_id] { cb(timer_id); });
    if (TRPC_UNLIKELY(!start_fiber)) {
      TRPC_ASSERT(false && "StartFiberDetached to execute fiber faild,abort!!!");
    }
  };

  auto sg = fiber::detail::NearestSchedulingGroup();
  auto timer_id = sg->CreateTimer(at, std::move(mcb));
  sg->EnableTimer(timer_id);
  return timer_id;
}

std::uint64_t SetFiberTimer(std::chrono::steady_clock::time_point at,
                            std::chrono::nanoseconds interval, Function<void()>&& cb) {
  return SetFiberTimer(at, interval, [cb = std::move(cb)](auto) { cb(); });
}

std::uint64_t SetFiberTimer(std::chrono::steady_clock::time_point at,
                            std::chrono::nanoseconds interval, Function<void(std::uint64_t)>&& cb) {
  struct UserCallback {
    void Run(std::uint64_t tid) {
      if (!running.exchange(true, std::memory_order_acq_rel)) {
        cb(tid);
      }
      running.store(false, std::memory_order_relaxed);
      // Otherwise this call is lost. This can happen if user's code runs too
      // slowly. For the moment we left the behavior as unspecified.
    }
    Function<void(std::uint64_t)> cb;
    std::atomic<bool> running{};
  };

  auto ucb = std::make_shared<UserCallback>();
  auto ec =
      object_pool::LwSharedPtr(object_pool::lw_shared_ref_ptr, FiberExecutionContext::Current());
  ucb->cb = std::move(cb);

  auto mcb = [ucb, ec = std::move(ec)](auto tid) mutable {
    Fiber::Attributes attr;
    attr.execution_context = ec.Get();
    bool start_fiber = StartFiberDetached(std::move(attr), [ucb, tid] { ucb->cb(tid); });
    if (TRPC_UNLIKELY(!start_fiber)) {
      TRPC_ASSERT(false && "StartFiberDetached to execute fiber faild,abort!!!");
    }
  };

  auto sg = fiber::detail::NearestSchedulingGroup();
  auto timer_id = sg->CreateTimer(at, interval, std::move(mcb));
  sg->EnableTimer(timer_id);
  return timer_id;
}

void DetachFiberTimer(std::uint64_t timer_id) {
  return fiber::detail::SchedulingGroup::GetTimerOwner(timer_id)->DetachTimer(timer_id);
}

void SetFiberDetachedTimer(std::chrono::steady_clock::time_point at, Function<void()>&& cb) {
  DetachFiberTimer(SetFiberTimer(at, std::move(cb)));
}

void SetFiberDetachedTimer(std::chrono::steady_clock::time_point at,
                           std::chrono::nanoseconds interval, Function<void()>&& cb) {
  DetachFiberTimer(SetFiberTimer(at, interval, std::move(cb)));
}

void KillFiberTimer(std::uint64_t timer_id) {
  return fiber::detail::SchedulingGroup::GetTimerOwner(timer_id)->RemoveTimer(timer_id);
}

FiberTimerKiller::FiberTimerKiller() = default;

FiberTimerKiller::FiberTimerKiller(std::uint64_t timer_id) : timer_id_(timer_id) {}

FiberTimerKiller::FiberTimerKiller(FiberTimerKiller&& tk) noexcept : timer_id_(tk.timer_id_) {
  tk.timer_id_ = 0;
}

FiberTimerKiller& FiberTimerKiller::operator=(FiberTimerKiller&& tk) noexcept {
  Reset();
  timer_id_ = std::exchange(tk.timer_id_, 0);
  return *this;
}

FiberTimerKiller::~FiberTimerKiller() { Reset(); }

void FiberTimerKiller::Reset(std::uint64_t timer_id) {
  if (auto tid = std::exchange(timer_id_, 0)) {
    KillFiberTimer(tid);
  }
  timer_id_ = timer_id;
}

[[nodiscard]] std::uint64_t CreateFiberTimer(std::chrono::steady_clock::time_point at,
                                             Function<void(std::uint64_t)>&& cb) {
  return fiber::detail::NearestSchedulingGroup()->CreateTimer(at, std::move(cb));
}

[[nodiscard]] std::uint64_t CreateFiberTimer(std::chrono::steady_clock::time_point at,
                                             std::chrono::nanoseconds interval,
                                             Function<void(std::uint64_t)>&& cb) {
  return fiber::detail::NearestSchedulingGroup()->CreateTimer(at, interval, std::move(cb));
}

void EnableFiberTimer(std::uint64_t timer_id) {
  fiber::detail::SchedulingGroup::GetTimerOwner(timer_id)->EnableTimer(timer_id);
}

}  // namespace trpc
