//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/condition_variable.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/coroutine/fiber_condition_variable.h"

namespace trpc {

void FiberConditionVariable::notify_one() noexcept { return impl_.notify_one(); }

void FiberConditionVariable::notify_all() noexcept { return impl_.notify_all(); }

void FiberConditionVariable::wait(std::unique_lock<FiberMutex>& lock) { return impl_.wait(lock); }

}  // namespace trpc
