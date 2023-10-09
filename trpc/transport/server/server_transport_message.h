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

#include <cstdint>
#include <string>

#include "trpc/server/server_context.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/object_pool/object_pool.h"

namespace trpc {

/// @brief Server transport
/// @note  Only used inside the framework
struct alignas(8) STransportMessage {
  /// The context of the request/response
  ServerContextPtr context{nullptr};

  /// The serialized data of the response
  NoncontiguousBuffer buffer;
};

using STransportReqMsg = STransportMessage;
using STransportRspMsg = STransportMessage;

/// @brief The data structure of actively close  the connection
struct alignas(8) CloseConnectionInfo {
  uint64_t connection_id = 0;
  std::string client_ip = "";
  uint16_t client_port = 0;
  int fd = 0;
};

namespace object_pool {

template <>
struct ObjectPoolTraits<STransportMessage> {
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
