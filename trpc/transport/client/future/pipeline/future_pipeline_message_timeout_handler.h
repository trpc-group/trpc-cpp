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

namespace trpc {

/// @brief Request timeout handler for pipeline scenario.
class FuturePipelineMessageTimeoutHandler {
 public:
  /// @brief Constructor.
  /// @param rsp_dispatch_function Function to dispatch response to handle threads.
  explicit FuturePipelineMessageTimeoutHandler(const TransInfo::RspDispatchFunction& rsp_dispatch_function);

  /// @brief Destructor.
  ~FuturePipelineMessageTimeoutHandler();

  /// @brief Push request to send queue.
  /// @param req_msg Pointer to request message.
  /// @return true: success, false: failed.
  bool PushToSendQueue(CTransportReqMsg* req_msg);

  /// @brief Pop request message from send queue.
  /// @return Pointer to request message, nullptr if not found.
  CTransportReqMsg* PopFromSendQueue();

  /// @brief Pop request message from send queue by requset id.
  /// @param request_id Request id.
  /// @return Pointer to request message, nullptr if not found.
  CTransportReqMsg* PopFromSendQueueById(uint32_t request_id);

  /// @brief Handle reqeust timeout in shared send queue.
  /// @return true: do timeout handling, false: not timeout handling.
  bool HandleSendQueueTimeout();

  /// @brief Check if send queue is empty or not.
  /// @return true: empty, false: not empty.
  bool IsSendQueueEmpty() { return send_queue_.Size() == 0; }

 private:
  // Limit max size of send queue.
  static constexpr size_t kSendQueueSize = 50000;

  // Function to dispatch response to handle threads.
  const TransInfo::RspDispatchFunction& rsp_dispatch_function_;

  // Send queue.
  internal::TimingWheelTimeoutQueue send_queue_;

  // Function for request timeout handling in pending queue.
  Function<void(const internal::TimingWheelTimeoutQueue::DataIterator&)> timeout_handle_function_;

  // Indicates whether a timeout has occurred.
  bool has_timeout_{false};
};

}  // namespace trpc
