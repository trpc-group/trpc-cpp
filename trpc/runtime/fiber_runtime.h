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

#include <cstdint>
#include <cstdlib>
#include <string>

#include "trpc/runtime/threadmodel/thread_model.h"

/// @brief Namespace of fiber runtime.
namespace trpc::fiber {

/// @private
namespace detail {

class SchedulingGroup;

/// @brief get the corresponding `SchedulingGroup` by index
/// @param index index
/// @note Preconditions: `index` < `GetSchedulingGroupCount()`
SchedulingGroup* GetSchedulingGroup(std::size_t index);

/// @brief get the `SchedulingGroup` closest to the calling thread
/// @return - if the calling thread is a `FiberWorker`, its corresponding `SchedulingGroup` will be returned
///         - if `NUMA aware` is turned on, `SchedulingGroup` on the same Node will be returned randomly
///         - if there is no `SchedulingGroup` in the current Node, and `NUMA aware` is not enabled,
///           `SchedulingGroup` will be returned randomly
///         - if no `SchedulingGroup` exists, `nullptr` will be returned
SchedulingGroup* NearestSchedulingGroup();

/// @brief same as `NearestSchedulingGroup()`
///        the difference is that the function returns the index of the `SchedulingGroup`
/// @return >=0 `SchedulingGroup` index,
///         -1 indicates that it does not exist
std::ptrdiff_t NearestSchedulingGroupIndex();

}  // namespace detail

/// @brief framework use. initialize and start fiber runtime environment
void StartRuntime();

/// @brief framework use. stop and destroy fiber runtime environment
void TerminateRuntime();

/// @brief get fiber threadmodel
ThreadModel* GetFiberThreadModel();

/// @brief get number of `SchedulingGroup` after fiber runtime started
/// @return number of `SchedulingGroup`
std::size_t GetSchedulingGroupCount();

/// @brief get number of `FiberWorker` after fiber runtime started
/// @return number of `FiberWorker`
std::size_t GetConcurrencyHint();

/// @brief get `SchedulingGroup` index the caller thread / fiber is currently belonging to
/// @return `SchedulingGroup` index
/// @note the behavior is undefined when calling this method outside of any `SchedulingGroup`
std::size_t GetCurrentSchedulingGroupIndex();

/// @brief get the `SchedulingGroup` size.
/// @return `SchedulingGroup` size
std::size_t GetSchedulingGroupSize();

/// @brief get NUMA node id by a given `SchedulingGroup` index
/// @return node id
/// @note this method only makes sense if `NUMA aware` is enabled
///       otherwise 0 is returned
std::size_t GetSchedulingGroupAssignedNode(std::size_t sg_index);

/// @brief traverse all `SchedulingGroup` to get the size of the fibers to be run in the run queue
std::size_t GetFiberQueueSize();

}  // namespace trpc::fiber
