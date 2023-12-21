//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string_view>

#include "trpc/util/align.h"
#include "trpc/util/function.h"
#include "trpc/util/thread/spinlock.h"

namespace trpc::fiber::detail {

struct RunnableEntity;
struct FiberDesc;
struct FiberEntity;
class SchedulingGroup;

/// @brief Base class of scheduling used for schedule the execution of fiber tasks.
///        The pure virtual interface must be implemented for internal fiber scheduling/execution within a scheduling
///        group. 'AddForeignSchedulingGroup' and 'StealFiberFromForeignSchedulingGroup' are optional interfaces that
///        need to be implemented for stealing fibers across scheduling groups.
class alignas(hardware_destructive_interference_size) Scheduling {
 public:
  virtual ~Scheduling() = default;

  /// @brief Initialize the scheduling.
  /// @param scheduling_group the scheduling group to which this scheduler belongs
  /// @param scheduling_group_size the number of worker threads in the scheduling group
  virtual bool Init(SchedulingGroup* scheduling_group,
                    std::size_t scheduling_group_size) noexcept = 0;

  /// @brief Add an foreign scheduling group for task stealing, which needs to be called before starting the fiber
  ///        worker.
  /// @param worker_index logical id of fiber worker thread
  /// @param sg foreign scheduling group
  /// @param steal_every_n task stealing factor
  virtual void AddForeignSchedulingGroup(std::size_t worker_index, SchedulingGroup* sg,
                                         std::uint64_t steal_every_n) noexcept {}

  /// @brief Steal fiber task from foreign scheduling group
  /// @return stolen fiber task, if failed, return nullptr
  virtual FiberEntity* StealFiberFromForeignSchedulingGroup() noexcept { return nullptr; }

  /// @brief For foregin scheduling group to acquire fiber task in this scheduling group
  /// @return stolen fiber task, if failed, return nullptr
  virtual FiberEntity* RemoteAcquireFiber() noexcept { return nullptr; }

  /// @brief Workers (including timer worker) call this to join this scheduling group.
  /// @param index logical id of fiber worker thread
  virtual void Enter(std::size_t index) noexcept = 0;

  /// @brief Fiber task scheduling action for worker thread
  virtual void Schedule() noexcept = 0;

  /// @brief You're calling this on thread exit.
  virtual void Leave() noexcept = 0;

  /// @brief Start and run one fiber task
  virtual bool StartFiber(FiberDesc* desc) noexcept = 0;

  /// @brief Schedule fibers in [start, end) to run in batch
  /// @param start start the beginning range (include)
  /// @param end the ending range (exclude)
  virtual void StartFibers(FiberDesc** start, FiberDesc** end) noexcept = 0;

  /// @brief Suspend the execution of the current fiber, which need to be woken by someone else explicitly via 'Resume'
  /// @param self the current fiber
  /// @param scheduler_lock lock of scheduler
  /// @note The 'FiberEntity::scheduler_lock' of the fiber must be held by the caller
  ///       This behavior can help prevent a race condition between calling this method and the 'Resume' method
  virtual void Suspend(FiberEntity* self, std::unique_lock<Spinlock>&& scheduler_lock) noexcept = 0;

  /// @brief Resumes the execution of the fiber, used in conjunction with 'Suspend'
  /// @param to The fiber to be resumed for execution
  virtual void Resume(FiberEntity* to) noexcept = 0;

  /// @brief Resumes the execution of the fiber, used in conjunction with 'Suspend'
  /// @param self The currently executing fiber, will be rescheduled
  /// @param to The fiber to be resumed for execution
  virtual void Resume(FiberEntity* self, FiberEntity* to) noexcept = 0;

  /// @brief Yield pthread worker to someone else.
  ///        The caller must not be added to run queue by anyone else (either concurrently or prior to this call).
  ///        It will be added to run queue by this method.
  /// @note `self->scheduler_lock` must NOT be held.
  virtual void Yield(FiberEntity* self) noexcept = 0;

  /// @brief Yield pthread worker to the specified fiber.
  ///        Both `self` and `to` must not be added to run queue by anyone else (either concurrently or prior to this
  ///        call). They'll be add to run queue by this method.
  /// @note  Scheduler lock of `self` and `to` must NOT be held.
  virtual void SwitchTo(FiberEntity* self, FiberEntity* to) noexcept = 0;

  /// @brief Stop the scheduling
  virtual void Stop() noexcept = 0;

  /// @brief Get the size of fiber task in current scheduling groups's task queue
  virtual std::size_t GetFiberQueueSize() noexcept = 0;
};

// Set the size of the fiber's run queue.
void SetFiberRunQueueSize(uint32_t queue_size);
// Get the size of the fiber's run queue.
uint32_t GetFiberRunQueueSize();

using SchedulingCreateFunction = Function<std::unique_ptr<Scheduling>()>;

constexpr std::string_view kSchedulingV1 = "v1";
constexpr std::string_view kSchedulingV2 = "v2";

// Initializes and loads the specific implementation of various fiber schedulers.
void InitSchedulingImp();

// Registers a fiber scheduler.
bool RegisterSchedulingImpl(std::string_view name, SchedulingCreateFunction&& func);

// Creates a specific fiber scheduler based on its name.
std::unique_ptr<Scheduling> CreateScheduling(std::string_view name);

}  // namespace trpc::fiber::detail
