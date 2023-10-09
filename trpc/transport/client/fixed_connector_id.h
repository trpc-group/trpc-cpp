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

#include "trpc/util/object_pool/object_pool_ptr.h"
#include "trpc/util/thread/spinlock.h"

namespace trpc {

/// @brief Id for fixed connection.
struct alignas(hardware_destructive_interference_size) FixedConnectorId
    : public object_pool::EnableLwSharedFromThis<FixedConnectorId> {
  // Which connector group belongs to.
  void* connector_group{nullptr};

  // Internal logic id.
  uint64_t conn_id{0};

  // Map to io thread id, used by future transport.
  uint16_t adapter_id{0};

  // Used by fiber transport.
  Spinlock lk;
};

namespace object_pool {

template <>
struct ObjectPoolTraits<FixedConnectorId> {
#if defined(TRPC_DISABLED_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kDisabled;
#elif defined(TRPC_SHARED_NOTHING_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
#else
  static constexpr auto kType = ObjectPoolType::kGlobal;
#endif
};

}  // namespace object_pool

}  // namespace trpc
