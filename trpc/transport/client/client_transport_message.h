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

#include "trpc/client/client_context.h"
#include "trpc/future/future.h"
#include "trpc/util/buffer/buffer.h"
#include "trpc/util/object_pool/object_pool_ptr.h"

namespace trpc {

/// @brief The info of dispatch the response message in async-future transport
/// @note  Ensure that response processing and request sending are on the same thread
struct ThreadModelDispatchInfo {
  /// The thread_model id of request sending
  int src_thread_model_id{-1};

  /// The thread id of request sending
  int src_thread_id{-1};

  /// The thread id of response processing
  int dst_thread_key{-1};
};

struct CTransportRspMsg;

/// @brief Client extend info for the request
struct ClientExtendInfo : public object_pool::LwSharedPtrCountBase {
  /// Which connection to send this request.
  uint64_t connection_id = 0;

  /// The promise of response message, use in asyc future transport
  Promise<CTransportRspMsg> promise;

  /// The backup-request promise
  void* backup_promise{nullptr};

  /// Is it a synchronous blocking call.
  /// When calling synchronous RPC interfaces in IO/Handle merge or separate mode this field is true.
  bool is_blocking_invoke = false;

  /// The info of dispatch the response message
  ThreadModelDispatchInfo dispatch_info;
};

namespace object_pool {

template <>
struct ObjectPoolTraits<ClientExtendInfo> {
#if defined(TRPC_DISABLED_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kDisabled;
#elif defined(TRPC_SHARED_NOTHING_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
#else
  static constexpr auto kType = ObjectPoolType::kGlobal;
#endif
};

}  // namespace object_pool

using ClientExtendInfoPtr = object_pool::LwSharedPtr<ClientExtendInfo>;

/// @brief Request message of client transport
/// @note  Only used inside the framework
struct alignas(8) CTransportReqMsg {
  /// The context of the request/response
  ClientContextPtr context{nullptr};

  /// The serialized data of the request
  NoncontiguousBuffer send_data;

  /// Client extend info for the request
  ClientExtendInfoPtr extend_info{nullptr};
};

/// @brief Response message of client transport
/// @note  Only used inside the framework
struct alignas(8) CTransportRspMsg {
  std::any msg;
};

namespace object_pool {

template <>
struct ObjectPoolTraits<CTransportReqMsg> {
#if defined(TRPC_DISABLED_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kDisabled;
#elif defined(TRPC_SHARED_NOTHING_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
#else
  static constexpr auto kType = ObjectPoolType::kGlobal;
#endif
};

template <>
struct ObjectPoolTraits<CTransportRspMsg> {
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
