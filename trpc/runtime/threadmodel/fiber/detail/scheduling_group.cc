//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/scheduling_group.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/threadmodel/fiber/detail/scheduling_group.h"

#include <climits>
#include <memory>
#include <string>

#include "trpc/runtime/threadmodel/fiber/detail/scheduling/v1/scheduling_impl.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling/v2/scheduling_impl.h"
#include "trpc/util/log/logging.h"

namespace trpc::fiber::detail {

using namespace std::literals;

std::once_flag init_flag;

SchedulingGroup::SchedulingGroup(const std::vector<unsigned>& affinity,
                                 uint8_t size,
                                 std::string_view scheduling_name)
    : group_size_(size), affinity_(affinity) {
  TRPC_CHECK_LE(group_size_, 64,
                "We only support up to 64 workers in each scheduling group. "
                "Use more scheduling groups if you want more concurrency.");

  std::call_once(init_flag, InitSchedulingImp);

  scheduling_ = CreateScheduling(scheduling_name);
  TRPC_ASSERT(scheduling_->Init(this, group_size_));
}

void SchedulingGroup::EnterGroup(std::size_t index) {
  TRPC_CHECK(current_ == nullptr, "This pthread worker has already joined a scheduling group.");
  TRPC_CHECK(timer_worker_ != nullptr, "The timer worker is not available yet.");
  TRPC_CHECK(scheduling_ != nullptr, "The scheduling is not available yet.");

  // Initialize thread-local timer queue for this worker.
  timer_worker_->InitializeLocalQueue(index);

  current_ = this;

  // Initialize scheduling information of this worker.
  scheduling_->Enter(index);
}

void SchedulingGroup::AddForeignSchedulingGroup(std::size_t worker_index, SchedulingGroup* sg,
                                                std::uint64_t steal_every_n) noexcept {
  scheduling_->AddForeignSchedulingGroup(worker_index, sg, steal_every_n);
}

FiberEntity* SchedulingGroup::StealFiberFromForeignSchedulingGroup() noexcept {
  return scheduling_->StealFiberFromForeignSchedulingGroup();
}

FiberEntity* SchedulingGroup::RemoteAcquireFiber() noexcept {
  return scheduling_->RemoteAcquireFiber();
}

void SchedulingGroup::Schedule() noexcept {
  scheduling_->Schedule();
}

bool SchedulingGroup::StartFiber(FiberDesc* fiber) noexcept {
  if (!scheduling_->StartFiber(fiber)) {
    DestroyFiberDesc(fiber);
    return false;
  }

  return true;
}

void SchedulingGroup::StartFibers(FiberDesc** start, FiberDesc** end) noexcept {
  scheduling_->StartFibers(start, end);
}

void SchedulingGroup::Suspend(FiberEntity* self, std::unique_lock<Spinlock>&& scheduler_lock) noexcept {
  scheduling_->Suspend(self, std::move(scheduler_lock));
}

void SchedulingGroup::Resume(FiberEntity* to) noexcept {
  scheduling_->Resume(to);
}

void SchedulingGroup::Resume(FiberEntity* self, FiberEntity* to) noexcept {
  scheduling_->Resume(self, to);
}

void SchedulingGroup::Yield(FiberEntity* self) noexcept {
  scheduling_->Yield(self);
}

void SchedulingGroup::SwitchTo(FiberEntity* self, FiberEntity* to) noexcept {
  scheduling_->SwitchTo(self, to);
}

void SchedulingGroup::LeaveGroup() {
  TRPC_CHECK(current_ == this, "This pthread worker does not belong to this scheduling group.");
  current_ = nullptr;
}

std::size_t SchedulingGroup::GroupSize() const noexcept { return group_size_; }

const std::vector<unsigned>& SchedulingGroup::Affinity() const noexcept { return affinity_; }

void SchedulingGroup::SetTimerWorker(TimerWorker* worker) noexcept { timer_worker_ = worker; }

void SchedulingGroup::Stop() {
  TRPC_CHECK(scheduling_ != nullptr, "The scheduling is not available yet.");

  scheduling_->Stop();
}

}  // namespace trpc::fiber::detail
