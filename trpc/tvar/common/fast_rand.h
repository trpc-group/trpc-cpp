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

#include <cstdint>

namespace trpc::tvar {

/// @brief Generate an unsigned 64-bit random number inside [0, range) from thread-local seed.
/// @param range Upper limit, [0, range)
/// @return 0 when range is 0.
/// @note This can be used as an adapter for std::random_shuffle():
///       std::random_shuffle(myvector.begin(), myvector.end(), FastRandLessThan);
/// @private
uint64_t FastRandLessThan(uint64_t range);

}  // namespace trpc::tvar
