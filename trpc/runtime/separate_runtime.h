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

#include "trpc/runtime/iomodel/reactor/reactor.h"
#include "trpc/runtime/threadmodel/common/msg_task.h"
#include "trpc/runtime/threadmodel/common/timer_task.h"
#include "trpc/runtime/threadmodel/thread_model.h"

/// @brief Namespace of separate runtime.
namespace trpc::separate {

/// @brief Framework use. Initialize and start separate runtime environment
///        : io and handle operation run on the different thread
/// @private
void StartRuntime();

/// @brief Framework use. Stop and destroy separate runtime environment
/// @private
void TerminateRuntime();

/// @brief Framework use. Initialize and start admin runtime environment
/// @private
void StartAdminRuntime();

/// @brief Framework use. Stop and destroy admin runtime environment
/// @private
void TerminateAdminRuntime();

/// @brief randomly get a separate threadmodel
ThreadModel* RandomGetSeparateThreadModel();

/// @brief submit io task to threadmodel
/// @param thread_model[in] separate threadmodel
/// @param io_task[in] io task is created by `object_pool`, this function will responsible for its release
/// @return true/false
bool SubmitIoTask(ThreadModel* thread_model, MsgTask* io_task) noexcept;

/// @brief submit business handle task to threadmodel
/// @param thread_model[in] separate threadmodel
/// @param handle_task[in] handle task is created by `object_pool`, this function will responsible for its release
/// @return true/false
bool SubmitHandleTask(ThreadModel* thread_model, MsgTask* handle_task) noexcept;

/// @brief get all reactors under thread_model
std::vector<Reactor*> GetReactors(ThreadModel* thread_model) noexcept;

/// @brief Get the reactor under thread_model by id, id is -1 means random
Reactor* GetReactor(ThreadModel* thread_model, int id) noexcept;

}  // namespace trpc::separate
