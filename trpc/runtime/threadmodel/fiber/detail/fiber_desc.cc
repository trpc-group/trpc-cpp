//
//
// Copyright (C) 2021 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/fiber/detail/fiber_desc.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/runtime/threadmodel/fiber/detail/fiber_desc.h"

#include "trpc/runtime/threadmodel/fiber/detail/waitable.h"
#include "trpc/util/object_pool/object_pool_ptr.h"

namespace trpc::object_pool {

template <>
struct ObjectPoolTraits<trpc::fiber::detail::FiberDesc> {
#if defined(TRPC_DISABLED_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kDisabled;
#elif defined(TRPC_SHARED_NOTHING_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
#else
  static constexpr auto kType = ObjectPoolType::kGlobal;
#endif
};

}  // namespace trpc::object_pool

namespace trpc::fiber::detail {

FiberDesc::FiberDesc() { SetRuntimeTypeTo<FiberDesc>(); }

FiberDesc* NewFiberDesc() noexcept {
  return trpc::object_pool::New<FiberDesc>();
}

void DestroyFiberDesc(FiberDesc* desc) noexcept {
  trpc::object_pool::Delete<FiberDesc>(desc);
}

}  // namespace trpc::fiber::detail
