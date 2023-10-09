//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/mutex.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include "trpc/runtime/threadmodel/fiber/detail/waitable.h"

namespace trpc {

/// @brief Analogous to `std::mutex`, but it's for fiber.
/// @note  It only uses in fiber runtime.
using FiberMutex = ::trpc::fiber::detail::Mutex;

}  // namespace trpc
