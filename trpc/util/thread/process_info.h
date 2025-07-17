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

#include <stdint.h>

#include <functional>

namespace trpc {

/// @brief Whether the process is running in a container environment.
/// @return true: Running in a container; false: Not running in a container.
bool IsProcessInContainer();

/// @brief Get the available CPU quota (number of cores) for the process.
/// @param check_container Whether a process is running in a container environment
/// @return CPU quota (number of cores)
/// @note When running outside of a container, the available resources are determined by the machine's configuration.
///       When running inside a container, the available resources are determined by the container's quota.
double GetProcessCpuQuota(const std::function<bool()>& check_container = IsProcessInContainer);

/// @brief Get the available memory quota (in bytes) for a process
/// @param check_container Whether a process is running in a container environment
/// @return Memory quota (in bytes) 
/// @note When running outside of a container, the available resources are determined by the machine's configuration.
///       When running inside a container, the available resources are determined by the container's quota.
int64_t GetProcessMemoryQuota(const std::function<bool()>& check_container = IsProcessInContainer);

}  // namespace trpc
