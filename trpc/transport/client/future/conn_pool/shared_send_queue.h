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

#include <vector>

#include "trpc/transport/client/client_transport_message.h"
#include "trpc/transport/client/future/common/timingwheel.h"
#include "trpc/util/function.h"
#include "trpc/util/object_pool/object_pool.h"

namespace trpc::internal {

/// @brief The request timeout queue, which is used for timeout detection of requests in connection pool mode, is
///        implemented using std::vector and a custom implementation of the time wheel data structure.
/// @note  Not thread safe, only used by framework, not for users.
class SharedSendQueue {
 public:
  class DataType;

  struct DataType {
    /// Pointer to request message, not owned by us.
    CTransportReqMsg* msg{nullptr};
    /// The expired timeout node pointer of the request message, used for request timeout management.
    TimerNode<std::vector<DataType>>* time_node{nullptr};
  };

  using DataIterator = std::vector<DataType>::iterator;

  /// @param capacity The maximum number of connections allowed in a connection pool.
  explicit SharedSendQueue(size_t capacity) : capacity_(capacity) { send_queue_.assign(capacity, DataType{}); }

  /// @brief Insert client request message into timeout queue.
  /// @param index connection id: 0~N
  /// @param[in] msg client request message
  /// @param[in] expire_time_ms request expiration time in milliseconds
  /// @return On success return true, otherwise return false.
  bool Push(uint64_t index, CTransportReqMsg* msg, uint64_t expire_time_ms);

  /// @brief Pop the send message of a connection by connection id.
  /// @param index connection id: 0~N
  CTransportReqMsg* Pop(uint64_t index);

  /// @brief Get the send message of a connnection by connection id.
  /// @param index connection id: 0~N
  CTransportReqMsg* Get(uint64_t index);

  /// @brief Pop the message from send queue by iterator.
  /// @param iter The iterator of the request message in the queue.
  /// @return  The message corresponding to the iterator position.
  /// @note Use to To obtain the request message in the timeout_handle processing function.
  CTransportReqMsg* GetAndPop(const DataIterator& iter);

  /// @brief Check timeout, and run timeout handler if neccesary.
  /// @param[in] now_ms Current time in milliseconds
  /// @param[in] timeout_handle Handle funtion for request timeout.
  void DoTimeout(size_t now_ms, const Function<void(const DataIterator&)>& timeout_handle);

  /// @brief Obtain the connection id based on the iterator.
  /// @param iter The iterator of the request message in the queue.
  /// @return connection id: 0~N
  uint64_t GetIndex(const DataIterator& iter) {
    uint64_t index = iter - send_queue_.begin();
    return index;
  }

 private:
  // Send queue of request message, corresponding to the connections under the connection pool.
  std::vector<DataType> send_queue_;

  // Timing wheel to implement timeout.
  TimingWheel<std::vector<DataType>> timing_wheel_;

  // capacity of the send queue
  size_t capacity_;
};

}  // namespace trpc::internal

namespace trpc::object_pool {

template <>
struct ObjectPoolTraits<trpc::internal::TimerNode<std::vector<trpc::internal::SharedSendQueue::DataType>>> {
#if defined(TRPC_DISABLED_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kDisabled;
#elif defined(TRPC_SHARED_NOTHING_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
#else
  static constexpr auto kType = ObjectPoolType::kGlobal;
#endif
};

}  // namespace trpc::object_pool
