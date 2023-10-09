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
#include <string>
#include <vector>

namespace trpc {

/// @brief Try to set the CPU affinity of the current thread.
/// @param affinity cpu affinity
/// @return Return 0 on success, other values on failure.
int TrySetCurrentThreadAffinity(const std::vector<unsigned>& affinity);

/// @brief Set the CPU affinity of the current thread.
/// @param affinity cpu affinity
/// @note If set fails then it will trigger an assertion.
void SetCurrentThreadAffinity(const std::vector<unsigned>& affinity);

/// @brief Try to get the CPU affinity of the current thread.
/// @param[out] affinity cpu affinity of the current thread
/// @return Return 0 on success, other values on failure.
int TryGetCurrentThreadAffinity(std::vector<unsigned>& affinity);

/// @brief Get the CPU affinity of the current thread.
/// @return cpu affinity of the current thread
std::vector<unsigned> GetCurrentThreadAffinity();

/// @brief Set the current thread name displayed in the top command
/// @param name name of the current thread
/// @note If set fails then it will trigger an assertion.
void SetCurrentThreadName(const std::string& name);

/// @brief Parse the core binding configuration and check its validity.
/// @param [in] bind_core_conf configuration string
/// @param [out] bind_core_group store the set of logical CPU numbers that the thread needs to be bound to
/// @return Return true on success, otherwise return false.
bool ParseBindCoreConfig(const std::string& bind_core_conf, std::vector<uint32_t>& bind_core_group);

}  // namespace trpc
