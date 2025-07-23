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

#include <limits>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "trpc/transport/common/transport_message_common.h"
#include "trpc/util/align.h"
#include "trpc/util/object_pool/object_pool_ptr.h"

namespace trpc {

// @brief An interface representing the ability to issue a synchronous backup request (retry).
class BackupRequestRetryBase {
 public:
  virtual ~BackupRequestRetryBase() = default;
  // @brief Reports whether reply is ready (also ready status is set).
  virtual bool IsReplyReady() = 0;
  // @brief First, increase the counter of failure, then reports whether all requests are failed.
  virtual bool IsFailedCountUpToAll() = 0;
  // @brief Reports whether the RPC request is finished.
  virtual bool IsFinished() = 0;
  // @brief Set finished status for RPC request.
  virtual void SetFinished() = 0;
  // @brief Wait for the RPC request to end (default to wait forever).
  virtual bool Wait(uint32_t timeout = 0) { return true; }
};

// @brief Args for issuing a backup request.
struct alignas(8) BackupRequestRetryInfo : public object_pool::EnableLwSharedFromThis<BackupRequestRetryInfo> {
  // Backup addresses.
  std::vector<ExtendNodeAddr> backup_addrs;

  // Delay interval.
  uint32_t delay{0};

  // Count of issued backup request.
  uint32_t resend_count{0};

  // Retry times.
  // uint32_t retry_times{0};

  // Store the indexes of instance node who reply succeed.
  int succ_rsp_node_index{0};

  // A controller who issues synchronous backup requests.
  BackupRequestRetryBase* retry{nullptr};
};

namespace object_pool {

template <>
struct ObjectPoolTraits<BackupRequestRetryInfo> {
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
