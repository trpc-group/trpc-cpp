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

#include "trpc/runtime/iomodel/reactor/reactor.h"
#include "trpc/runtime/threadmodel/common/msg_task.h"
#include "trpc/runtime/threadmodel/common/timer_task.h"
#include "trpc/runtime/threadmodel/thread_model.h"

/// @brief Namespace of merge runtime.
namespace trpc::merge {

/// @brief Framework use. Initialize and start merge runtime environment
///        merge runtime: io and handle operation run on the same thread
/// @private
void StartRuntime();

/// @brief Framework use. Stop and destroy merge runtime environment
/// @private
void TerminateRuntime();

/// @brief randomly get a merge threadmodel
ThreadModel* RandomGetMergeThreadModel();

/// @brief submit io task to threadmodel
/// @param thread_model[in] merge threadmodel
/// @param io_task[in] io task is created by `object_pool`, this function will responsible for its release
/// @return true/false
bool SubmitIoTask(ThreadModel* thread_model, MsgTask* io_task) noexcept;

/// @brief submit business handle task to threadmodel
/// @param thread_model[in] merge threadmodel
/// @param handle_task[in] handle task is created by `object_pool`, this function will responsible for its release
/// @return true/false
bool SubmitHandleTask(ThreadModel* thread_model, MsgTask* handle_task) noexcept;

/// @brief get all reactors under thread_model
/// @private
std::vector<Reactor*> GetReactors(ThreadModel* thread_model) noexcept;

/// @brief Get the reactor under thread_model by id, id is -1 means random
/// @private
Reactor* GetReactor(ThreadModel* thread_model, int id) noexcept;

}  // namespace trpc::merge
