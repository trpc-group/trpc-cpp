//
//
// Copyright (C) 2020 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/chrono.h.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <atomic>
#include <chrono>
#include <thread>

#include "trpc/util/align.h"

namespace trpc {

/// @brief Same as `std::chrono::steady_clock::now()`, except that this one can be
///        faster if `std`'s counterpart is implemented in terms of `syscall` (this is
///        the case for libstdc++ `configure`d on CentOS6 with default options).
std::chrono::steady_clock::time_point ReadSteadyClock();

/// @brief Same as `std::chrono::system_clock::now()`.
/// @return `system_clock::time_point`
std::chrono::system_clock::time_point ReadSystemClock();

}  // namespace trpc
