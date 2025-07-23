//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/fiber_entity.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/threadmodel/fiber/detail/fiber_entity.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1  // `pthread_getattr_np` needs this.
#endif

#include <pthread.h>

#include <limits>
#include <utility>

#include "trpc/runtime/threadmodel/fiber/detail/fiber_id_gen.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling_group.h"
#include "trpc/runtime/threadmodel/fiber/detail/stack_allocator_impl.h"
#include "trpc/runtime/threadmodel/fiber/detail/waitable.h"
#include "trpc/util/deferred.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/string_helper.h"

// Defined in `trpc/threadmodel/fiber/detail/fcontext/{arch}/*.S`
extern "C" {

void* make_context(void* sp, std::size_t size, void (*start_proc)(void*));
}

namespace trpc::fiber::detail {

namespace {

const auto kFiberEntityToStackTopOffset = -kFiberMagicSize + kPageSize;
static std::atomic<std::uint32_t> fiber_count{0};

struct FiberIdTraits {
  using Type = std::uint64_t;
  static constexpr auto kMin = 1;
  static constexpr auto kMax = std::numeric_limits<std::uint64_t>::max();
  // I don't expect a pthread worker need to create more than 128K fibers per
  // sec.
  static constexpr auto kBatchSize = 131072;
};

#ifdef TRPC_INTERNAL_USE_ASAN

// Returns: stack bottom (lowest address) and limit of current pthread (i.e.
// master fiber's stack).
std::pair<const void*, std::size_t> GetMasterFiberStack() noexcept {
  pthread_attr_t self_attr;
  TRPC_PCHECK(pthread_getattr_np(pthread_self(), &self_attr) == 0);
  ScopedDeferred _(
      [&] { TRPC_PCHECK(pthread_attr_destroy(&self_attr) == 0); });

  void* stack;
  std::size_t limit;
  TRPC_PCHECK(pthread_attr_getstack(&self_attr, &stack, &limit) == 0);
  return {stack, limit};
}

#endif

}  // namespace

// Entry point for newly-started fibers.
//
// NOT put into anonymous namespace to simplify its displayed name in GDB.
//
// `extern "C"` doesn't help, unfortunately. (It does simplify mangled name,
// though.)
//
// Do NOT mark this function as `noexcept`. We don't want to force stack being
// unwound on exception.
static void FiberProc(void* context) {
  auto self = reinterpret_cast<FiberEntity*>(context);
  // We're running in `self`'s stack now.

#ifdef TRPC_INTERNAL_USE_ASAN
  // A new fiber has born, so complete with a new shadow stack.
  //
  // By passing `nullptr` to this call, a new shadow stack is allocated
  // internally. (@sa: `asan::CompleteSwitchFiber`).
  trpc::internal::asan::CompleteSwitchFiber(nullptr);
#endif

  SetCurrentFiberEntity(self);  // We're alive.
  self->state = FiberState::Running;

  // Hmmm, there is a pending resumption callback, even if we haven't completely
  // started..
  //
  // We'll run it anyway. This, for now, is mostly used for `Dispatch` fiber
  // launch policy.
  DestructiveRunCallbackOpt(&self->resume_proc);
  DestructiveRunCallback(&self->start_proc);

  // We're leaving now.
  TRPC_CHECK_EQ(self, GetCurrentFiberEntity());

  // This fiber should not be waiting on anything (mutex / condition_variable
  // / ...), i.e., no one else should be referring this fiber (referring to its
  // `exit_barrier` is, since it's ref-counted, no problem), otherwise it's a
  // programming mistake.

  // Let's see if there will be someone who will be waiting on us.
  if (!self->exit_barrier) {
    // Mark the fiber as dead. This prevent our GDB plugin from listing this
    // fiber out.
    self->state = FiberState::Dead;

#ifdef TRPC_INTERNAL_USE_ASAN
    // We're leaving, a special call to asan is required. So take special note
    // of it.
    //
    // Consumed by `FiberEntity::Resume()` prior to switching stack.
    self->asan_terminating = true;
#endif

    // No one is waiting for us, this is easy.
    GetMasterFiberEntity()->ResumeOn([self] { FreeFiberEntity(self); });
  } else {
    // The lock must be taken first, we can't afford to block when we (the
    // callback passed to `ResumeOn()`) run on master fiber.
    //
    // CAUTION: WE CAN TRIGGER RESCHEDULING HERE.
    auto ebl = self->exit_barrier->GrabLock();

    // Must be done after `GrabLock()`, as `GrabLock()` is self may trigger
    // rescheduling.
    self->state = FiberState::Dead;

#ifdef TRPC_INTERNAL_USE_ASAN
    self->asan_terminating = true;
#endif

    // We need to switch to master fiber and free the resources there, there's
    // no call-stack for us to return.
    GetMasterFiberEntity()->ResumeOn([self, lk = std::move(ebl)]() mutable {
      // The `exit_barrier` is move out so as to free `self` (the stack)
      // earlier. Stack resource is precious.
      auto eb = std::move(self->exit_barrier);

      // Because no one else if referring `self` (see comments above), we're
      // safe to free it here.
      FreeFiberEntity(self);  // Good-bye.

      // Were anyone waiting on us, wake them up now.
      eb->UnsafeCountDown(std::move(lk));
    });
  }
  TRPC_CHECK(0);  // Can't be here.
}

FiberEntity::FiberEntity() { SetRuntimeTypeTo<FiberEntity>(); }

void FiberEntity::ResumeOn(Function<void()>&& cb) noexcept {
  auto caller = GetCurrentFiberEntity();
  TRPC_CHECK(!resume_proc,
              "You may not call `ResumeOn` on a fiber twice (before the first "
              "one has executed).");
  TRPC_CHECK_NE(caller, this, "Calling `Resume()` on self is undefined.");

  // This pending call will be performed and cleared immediately when we
  // switched to `*this` fiber (before calling user's continuation).
  resume_proc = std::move(cb);
  Resume();
}

ErasedPtr* FiberEntity::GetFlsSlow(std::size_t index) noexcept {
  TRPC_FMT_WARN_ONCE("Excessive FLS usage. Performance will likely degrade.");
  if (TRPC_UNLIKELY(!external_fls)) {
    external_fls =
        std::make_unique<std::unordered_map<std::size_t, ErasedPtr>>();
  }
  return &(*external_fls)[index];
}

FiberEntity::trivial_fls_t* FiberEntity::GetTrivialFlsSlow(
    std::size_t index) noexcept {
  TRPC_FMT_WARN_ONCE("Excessive FLS usage. Performance will likely degrade.");
  if (TRPC_UNLIKELY(!external_trivial_fls)) {
    external_trivial_fls =
        std::make_unique<std::unordered_map<std::size_t, trivial_fls_t>>();
  }
  return &(*external_trivial_fls)[index];
}

void SetUpMasterFiberEntity() noexcept {
  thread_local FiberEntity master_fiber_impl;

  master_fiber_impl.debugging_fiber_id = -1;
  master_fiber_impl.state_save_area = nullptr;
  master_fiber_impl.state = FiberState::Running;
  master_fiber_impl.stack_size = 0;

  master_fiber_impl.scheduling_group = SchedulingGroup::Current();

#ifdef TRPC_INTERNAL_USE_ASAN
  std::tie(master_fiber_impl.asan_stack_bottom,
           master_fiber_impl.asan_stack_size) = GetMasterFiberStack();
#endif

#ifdef TRPC_INTERNAL_USE_TSAN
  master_fiber_impl.tsan_fiber = trpc::internal::tsan::GetCurrentFiber();
#endif

  master_fiber = &master_fiber_impl;
  SetCurrentFiberEntity(master_fiber);
}

#if defined(TRPC_INTERNAL_USE_TSAN) || defined(__powerpc64__) || \
    defined(__aarch64__)

FiberEntity* GetMasterFiberEntity() noexcept { return master_fiber; }

FiberEntity* GetCurrentFiberEntity() noexcept { return current_fiber; }

void SetCurrentFiberEntity(FiberEntity* current) { current_fiber = current; }

#endif

FiberEntity* CreateFiberEntity(SchedulingGroup* sg, Function<void()>&& start_proc) noexcept {
  auto desc = NewFiberDesc();
  desc->scheduling_group_local = false;
  desc->start_proc = std::move(start_proc);

  return InstantiateFiberEntity(sg, desc);
}

FiberEntity* InstantiateFiberEntity(SchedulingGroup* sg, FiberDesc* desc) noexcept {
  ScopedDeferred _{[&] { DestroyFiberDesc(desc); }};

  void* stack = nullptr;
  bool is_from_system = true;

  bool succ = Allocate(&stack, &is_from_system);
  if (TRPC_UNLIKELY(!succ)) {
    TRPC_FMT_INFO_EVERY_SECOND("CreateFiberEntity failed.");
    TRPC_ASSERT(false);

    return nullptr;
  }

  fiber_count.fetch_add(1, std::memory_order_relaxed);
  ReuseStackInSanitizer(stack);

  auto ptr = reinterpret_cast<char*>(stack) + GetFiberStackSize() - kPageSize;
  TRPC_DCHECK_EQ(reinterpret_cast<std::uintptr_t>(ptr) & (kPageSize - 1), std::uintptr_t(0));
  TRPC_DCHECK_LE(sizeof(FiberEntity), kPageSize);
  // NOT value-initialized intentionally, to save precious CPU cycles.
  auto fiber = new (ptr + kFiberMagicSize) FiberEntity;  // A new life has born.

  fiber->debugging_fiber_id = Next<FiberIdTraits>();
  fiber->state = FiberState::Ready;
  fiber->scheduling_group = sg;
  fiber->stack_size = GetFiberStackSize();
  fiber->state_save_area = make_context(fiber->GetStackTop(), fiber->GetStackLimit(), FiberProc);
  fiber->is_from_system = is_from_system;
  fiber->start_proc = std::move(desc->start_proc);
  fiber->exit_barrier = std::move(desc->exit_barrier);
  fiber->last_ready_tsc = desc->last_ready_tsc;
  fiber->scheduling_group_local = desc->scheduling_group_local;
  fiber->is_fiber_reactor = desc->is_fiber_reactor;

#ifdef TRPC_INTERNAL_USE_ASAN
  fiber->asan_stack_bottom = stack;
  fiber->asan_stack_size = fiber->GetStackLimit();
#endif

#ifdef TRPC_INTERNAL_USE_TSAN
  fiber->tsan_fiber = trpc::internal::tsan::CreateFiber();
#endif

  return fiber;
}

void FreeFiberEntity(FiberEntity* fiber) noexcept {
  bool is_from_system = fiber->is_from_system;
  uint32_t fiber_stack_size = fiber->stack_size;

  fiber_count.fetch_sub(1, std::memory_order_relaxed);
  fiber->ever_started_magic = 0;  // Hopefully the compiler does not optimize
                                  // this away.

#ifdef TRPC_INTERNAL_USE_TSAN
  trpc::internal::tsan::DestroyFiber(fiber->tsan_fiber);
#endif

  fiber->chain.next = fiber->chain.prev = &fiber->chain;
  fiber->~FiberEntity();

  auto p = reinterpret_cast<char*>(fiber) + kFiberEntityToStackTopOffset - fiber_stack_size;
  TRPC_DCHECK_EQ(reinterpret_cast<std::uintptr_t>(p) & (kPageSize - 1), std::uintptr_t(0));
  Deallocate(p, is_from_system);
}

uint32_t GetFiberCount() {
  return fiber_count.load(std::memory_order_relaxed);
}

}  // namespace trpc::fiber::detail
