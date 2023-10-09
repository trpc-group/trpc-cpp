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

/// @brief Round up to the nearest power of 2.
/// @param n input data.
/// @return the smallest power of 2 greater than n.
std::size_t RoundUpPowerOf2(std::size_t n);

}  // namespace trpc
