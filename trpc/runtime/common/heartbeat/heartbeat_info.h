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

#include <sched.h>

#include <cstdint>
#include <string>

#include "trpc/util/function.h"

namespace trpc {

/// @brief service-level heartbeat information, used to do heartbeat reporting to the naming service.
struct ServiceHeartBeatInfo {
  /// group_name of thread model
  std::string group_name;

  /// service name that registered in naming service
  std::string service_name;

  /// instance's ip that registered in naming service
  std::string host;

  /// instance's port that registered in naming service
  int port{0};
};

/// @brief Worker thread's heartbeat information, used to detect whether a thread is deadlocked and do thread's task
///        queue size reporting. It't used by framework only.
/// @note  Use group_id(group_name) + thread_role + thread_id to determine a thread
///
///        The hierarchical relationship is as follows:
///                       group_id
///                     *          *
///             thread_role      thread_role
///             *        *       *        *
///        thread_id thread_id thread_id thread_id
///
///        About deadlocks detection, please note:
///        1. If all thread in a certain role of the thread model instance are deadlocked, then the thread model
///           instance is considered deadlocked.
///        2. If there are no threads under a certain role, it will not be considered deadlocked.
///
///        In current implementation:
///        1. Separate thread model
///           Due to the Io and Handle threads under the thread group are separated, if any threads under Io or Handle
///           role are all deadlocked, then the thread model instance is considered deadlocked.
///        2. Merge thread model and fiber m:n coroutine thread model
///           Due to the threads under the thread group don't distinguish between io and handle roles, if all threads
///           are deadlocked, then the thread model instance is considered deadlocked.
/// @private
struct ThreadHeartBeatInfo {
  /// group_name of thread model
  std::string group_name;

  /// role of worker thread, such as io, handle, io_and_handle
  uint16_t role;

  /// unique logic id of worker thread
  uint32_t id{0};

  /// pid of worker thread
  pid_t pid{0};

  /// time of the last heartbeat report.
  uint64_t last_time_ms{0};

  /// current task queue size of worker thread
  uint32_t task_queue_size{0};
};

/// Function that used for collect thread's heartbeat information
/// @private
using ThreadHeartBeatFunction = Function<void(ThreadHeartBeatInfo&&)>;

/// @brief Framework use or for testing. Register function that used for collect thread's heartbeat information.
///        It is not thread-safe and it's called by 'HeartBeatReport' class when framework is initialized.
/// @private
void RegisterThreadHeartBeatFunction(ThreadHeartBeatFunction&& heartbeat_function);

/// @brief Framework use or for testing. It't Call by worker thread to report heartbeat information, thread-safe.
void HeartBeat(uint32_t task_queue_size);

}  // namespace trpc
