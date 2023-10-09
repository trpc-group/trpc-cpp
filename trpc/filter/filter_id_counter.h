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

namespace trpc {

/// @brief Get the next available filter ID and increments the filter_id_counter atomically.
/// @return The next available filter ID.
uint16_t GetNextFilterID();

// Unit test use only, users are forbidden to use
void SetFilterIDCounter(uint16_t value);

} // namespace trpc
