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

/// @brief Get the hash value corresponding to the key value
/// @param x Key value
/// @return Hash value
std::size_t GetHashValue(std::uint64_t x);

/// @brief Get the hash index
/// @param x Key value
/// @param mod mod value
/// @return Hash index
std::size_t GetHashIndex(std::uint64_t x, std::uint64_t mod);

}  // namespace trpc
