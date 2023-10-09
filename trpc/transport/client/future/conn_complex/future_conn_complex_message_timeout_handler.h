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

#include "trpc/transport/client/client_transport_message.h"
#include "trpc/transport/client/future/common/timingwheel_timeout_queue.h"
#include "trpc/transport/client/trans_info.h"
#include "trpc/util/function.h"

namespace trpc {

/// @brief Request timeout handler for connection complex scenario.
class FutureConnComplexMessageTimeoutHandler {
 public:
  /// @brief Result of pushing request into timeout queue.
  enum PushResult {
    // Failed.
    kError = -1,
    // Success.
    kOk = 0,
    // Used for backup request, as requests to different backends in the same group manager
    // share same timeout queue, same request id may exists in the timeout queue already, there
    // is no need to push request to timeout queue anymore, but memory of request need to be
    // freed after backup request sent.
    kExisted = 1,
  };

  /// @brief Constructor.
  /// @param rsp_dispatch_function Function to dispatch response to handle threads.
  explicit FutureConnComplexMessageTimeoutHandler(const TransInfo::RspDispatchFunction& rsp_dispatch_function);

  /// @brief Push request into timeout queue.
  /// @param req_msg Pointer to request message.
  /// @return PushResult above.
  PushResult Push(CTransportReqMsg* req_msg);

  /// @brief Remove and return request from timeout queue by request id.
  /// @param request_id Request id.
  /// @return Pointer to request message, nullptr if not found.
  CTransportReqMsg* Pop(uint32_t request_id);

  /// @brief Get request from timeout queue by request id without deleting.
  /// @param request_id Request id.
  /// @return Pointer to request message, nullptr if not found.
  CTransportReqMsg* Get(uint32_t request_id);

  /// @brief Check for request timeout in timeout queue, handle if neccesary.
  void DoTimeout();

  /// @brief Get how many requests inside timeout queue.
  size_t GetTimeoutQueueSize() { return timeout_queue_.Size(); }

  /// @brief Check for timeout queue empty or not.
  /// @return true: empty, false: not empty.
  bool IsTimeoutQueueEmpty() { return timeout_queue_.Size() == 0; }

 private:
  // Size of hash buckets in timeout queue.
  static constexpr uint32_t kTimeoutQueueHashBucketSize = 100000;

  // Limit timeout queue request number, drop request if exceed.
  static constexpr size_t kTimeoutQueueLimitSize = 50000;

  // Timeout queue, map from request id to message request.
  // Use request id as unique id in connection complex scenario.
  internal::TimingWheelTimeoutQueue timeout_queue_;

  // Function for request timeout handling.
  Function<void(const internal::TimingWheelTimeoutQueue::DataIterator&)> timeout_handle_function_;

  // Fucntion to dispatch response to handle threads.
  const TransInfo::RspDispatchFunction& rsp_dispatch_function_;
};

}  // namespace trpc
