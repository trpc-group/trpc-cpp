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

#include <functional>

#include "trpc/runtime/runtime_compatible.h"

/// @brief Namespace of trpc runtime.
namespace trpc::runtime {

/// @brief pure thread runtime environment flag
///        it indicates that the main logic of the framework run on
///        the merge threadmodel(io and handle on the same thread) or
///        on the separate threadmodel(io and handle in the different thread)
constexpr uint32_t kThreadRuntime = 0;

/// @brief fiber(m:n coroutine) flag
///        it indicates that the main logic of the framework runs in the fiber runtime environment
constexpr uint32_t kFiberRuntime = 1;

/// @brief get the type of framework runtime
/// @note default it return kThreadRuntime or kFiberRuntime
///       if self-define implement runtime, return the self-define type
uint32_t GetRuntimeType();

/// @brief set the type of framework runtime
void SetRuntimeType(uint32_t type);

/// @brief determine whether the framework is running in the fiber runtime
bool IsInFiberRuntime();

/// @brief framework use. initialize and start the framework's runtime environment
/// @private
bool StartRuntime();

/// @brief framework use. run the main body of the framework to run the logic code
/// @private
bool Run(std::function<void()>&& func);

/// @brief framework use. stop and destroy the framework's runtime environment
/// @private
void TerminateRuntime();

/// @brief framework use. Init fiber reactor config
/// @private
void InitFiberReactorConfig();

}  // namespace trpc::runtime
