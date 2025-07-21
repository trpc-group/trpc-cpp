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

#include <cstdint>

#include "trpc/transport/client/client_transport_message.h"
#include "trpc/transport/client/future/common/timingwheel_timeout_queue.h"
#include "trpc/transport/client/future/conn_pool/shared_send_queue.h"
#include "trpc/transport/client/trans_info.h"

namespace trpc {

/// @brief Request timeout handler for connection pool scenario.
class FutureConnPoolMessageTimeoutHandler {
 public:
  /// @brief Constructor.
  /// @param conn_id Connector id.
  /// @param rsp_dispatch_function Function to dispatch response to handle threads.
  /// @param shared_send_queue Shared timeout queue belongs to connection pool.
  FutureConnPoolMessageTimeoutHandler(uint64_t conn_id, const TransInfo::RspDispatchFunction& rsp_dispatch_function,
                                      internal::SharedSendQueue& shared_send_queue);

  /// @brief Destructor.
  ~FutureConnPoolMessageTimeoutHandler();

  /// @brief Push request to shared send queue.
  /// @param req_msg Pointer to request message.
  /// @return true: success, false: failed.
  bool PushToSendQueue(CTransportReqMsg* req_msg);

  /// @brief Pop request message belongs to current connection from shared send queue.
  /// @return Pointer to request message, nullptr if not found.
  CTransportReqMsg* PopFromSendQueue();

  /// @brief Get request message belongs to current connection from shared send queue, without deleting it.
  /// @return Pointer to request message, nullptr if not found.
  const CTransportReqMsg* GetSendMsg();

  /// @brief Push request to pending queue.
  /// @param req_msg Pointer to request message.
  /// @return true: success, false: failed.
  bool PushToPendingQueue(CTransportReqMsg* req_msg);

  /// @brief Pop request message from pending queue.
  /// @return Pointer to request message, nullptr if not found.
  CTransportReqMsg* PopFromPendingQueue();

  /// @brief Handle reqeust timeout in shared send queue.
  /// @param msg Pointer to request message.
  /// @return true: do timeout handling, false: not timeout handling.
  bool HandleSendQueueTimeout(CTransportReqMsg* msg);

  /// @brief Handle reqeust timeout in pending queue.
  void HandlePendingQueueTimeout();

  /// @brief Check if there is no sent request belongs to current connection in shared send queue.
  /// @return true: no sent request found, false: sent request found.
  bool IsSendQueueEmpty() { return shared_send_queue_.Get(conn_id_) == nullptr; }

  /// @brief Check if there is no pending requests in pending queue.
  /// @return true: no request found, false: request found.
  bool IsPendingQueueEmpty() { return pending_queue_.Size() == 0; }

  /// @brief Set whether need to check queue to do timeout when exit.
  void NeedCheckQueueWhenExit(bool value) { need_check_queue_when_exit_ = value; }

 private:
  // Limit max size of pending queue.
  static constexpr size_t kPendingQueueSize = 5000;

  // Connector id, 0 to N.
  uint64_t conn_id_;

  // Function to dispatch response to handle threads.
  const TransInfo::RspDispatchFunction& rsp_dispatch_function_;

  // Reference to shared send queue.
  internal::SharedSendQueue& shared_send_queue_;

  // Pending queue belongs to current connection, for fixed-connection scenario.
  // Map from request id to request message.
  internal::SmallCacheTimingWheelTimeoutQueue pending_queue_;

  // Function for request timeout handling in pending queue.
  Function<void(const internal::SmallCacheTimingWheelTimeoutQueue::DataIterator&)> timeout_handle_function_;

  // Whether need to check queue to do timeout when exit.
  bool need_check_queue_when_exit_{true};
};

}  // namespace trpc
