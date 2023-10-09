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

#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/transport/client/client_transport_message.h"
#include "trpc/transport/client/retry_info_def.h"
#include "trpc/transport/client/trans_info.h"
#include "trpc/util/align.h"
#include "trpc/util/function.h"
#include "trpc/util/object_pool/object_pool_ptr.h"
#include "trpc/util/thread/spinlock.h"

namespace trpc {

/// @brief Callback processing after the response is returned
using OnCompletionFunction = Function<void(int, std::string&&)>;

/// @brief The context of the request call
struct alignas(hardware_destructive_interference_size) CallContext {
  Spinlock lock;
  uint32_t request_id{0};
  uint32_t request_merge_count{1};  // for redis pipeline command
  CTransportReqMsg* req_msg{nullptr};
  CTransportRspMsg* rsp_msg{nullptr};
  uint64_t timeout_timer{0};
  BackupRequestRetryInfo* backup_request_retry_info{nullptr};
  OnCompletionFunction on_completion_function{nullptr};
};

namespace object_pool {

template <>
struct ObjectPoolTraits<CallContext> {
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
