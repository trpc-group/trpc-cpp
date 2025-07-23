//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/execution_context.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/coroutine/fiber_execution_context.h"

#include <cstdint>

namespace trpc {

FiberLocal<FiberExecutionContext*> FiberExecutionContext::current_;

void FiberExecutionContext::Clear() {
  TRPC_CHECK_EQ(UseCount(), std::uint32_t(1),
                "Unexpected: `FiberExecutionContext` is using by others when `Clear()`-ed.");

  for (auto&& e : inline_els_) {
    e.~ElsEntry();
    new (&e) ElsEntry();
  }
  external_els_.clear();
}

object_pool::LwSharedPtr<FiberExecutionContext> FiberExecutionContext::Capture() {
  return object_pool::LwSharedPtr(object_pool::lw_shared_ref_ptr, *current_);
}

object_pool::LwSharedPtr<FiberExecutionContext> FiberExecutionContext::Create() {
  return object_pool::MakeLwShared<FiberExecutionContext>();
}

FiberExecutionContext::ElsEntry* FiberExecutionContext::GetElsEntrySlow(std::size_t slot_index) {
  TRPC_FMT_WARN_ONCE("Excessive ELS usage. Performance will likely degrade.");

  std::scoped_lock _(external_els_lock_);
  return &external_els_[slot_index];
}

}  // namespace trpc
