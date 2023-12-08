//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/waitable.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include "trpc/runtime/threadmodel/fiber/detail/waitable.h"

namespace trpc {

/// @brief Adaptive event primitive for both fiber and pthread context.
class FiberEvent {
 public:
  /// @brief Wait until `Set()` is called.
  ///        If `Set()` is called before `Wait()`, this method returns immediately.
  void Wait() { event_.Wait(); }

  /// @brief Wake up fibers and pthreads blockings on `Wait()`.
  void Set() { event_.Set(); }

 private:
  trpc::fiber::detail::Event event_;
};

}  // namespace trpc
