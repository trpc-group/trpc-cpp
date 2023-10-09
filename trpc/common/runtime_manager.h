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

namespace trpc {

/// @brief The entry function for the pure client program runs in trpc(fiber/merge/separate) runtime
/// @return 0: success, other: failed.
int RunInTrpcRuntime(std::function<int()>&& func);

/// @brief The entry function for the pure client program runs in fiber runtime
/// @return 0: success, other: failed.
int RunInFiberRuntime(std::function<int()>&& func);

/// @brief The entry function for the pure client program runs in thread(merge/separate) runtime
/// @return 0: success, other: failed.
int RunInThreadRuntime(std::function<int()>&& func);

/// @brief Whether to run in fiber(m:n coroutine) mode
/// @private For internal use purpose only.
bool IsInFiberRuntime();

/// @brief Initialize the runtime environment of the framework.
/// @not For compatible, please not use it.
/// @private For internal use purpose only.
int InitFrameworkRuntime();

/// @brief Destroy the runtime environment of the framework.
/// @note for compatible, please not use it.
/// @private For internal use purpose only.
void DestroyFrameworkRuntime();

}  // namespace trpc
