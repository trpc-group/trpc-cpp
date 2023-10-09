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

#include "trpc/transport/client/client_transport_message.h"
#include "trpc/transport/client/future/common/timingwheel.h"
#include "trpc/util/container/container.h"
#include "trpc/util/function.h"
#include "trpc/util/object_pool/object_pool.h"

namespace trpc::internal {

struct HashMapValue;
constexpr std::size_t kCacheItemSize = 10000;

/// @brief Map from request id to HashMapValue.
using HashMap = trpc::container::FixedUnorderedMap<uint32_t, HashMapValue, kCacheItemSize>;

/// @brief Wrap request message and TimerNode.
struct HashMapValue {
  /// Pointer to request message, not owned by us.
  CTransportReqMsg* msg{nullptr};
  /// The expired timeout node pointer of the request message, used for request timeout management.
  TimerNode<HashMap>* time_node{nullptr};
};

struct SmallCacheHashMapValue;
constexpr std::size_t kSmallCacheItemSize = 500;

/// @brief Same as HashMap with the smaller cache item size.
/// @note The purpose of this queue is to reduce memory usage.
using SmallCacheHashMap = trpc::container::FixedUnorderedMap<uint32_t, SmallCacheHashMapValue, kSmallCacheItemSize>;

/// @brief Wrap request message and TimerNode.
struct SmallCacheHashMapValue {
  /// Pointer to request message, not owned by us.
  CTransportReqMsg* msg{nullptr};
  /// For request timeout.
  TimerNode<SmallCacheHashMap>* time_node{nullptr};
};

/// @brief The request timeout queue, designed for timeout detection, is implemented using std::unordered_map and a
///        custom implementation of the time wheel data structure.
/// @note  Not thread safe, only used by framework, not for users.
template <typename T, typename V>
class TimingWheelTimeoutQueueImpl {
 public:
  using DataIterator = typename T::iterator;

  /// @brief Constructor.
  explicit TimingWheelTimeoutQueueImpl(size_t hash_bucket_count = 0) {
    if (hash_bucket_count != 0) {
      hash_map_.reserve(hash_bucket_count);
    }
  }

  /// @brief Insert client request message into timeout queue.
  /// @param unique_id Unique id to identify this request.
  /// @param msg Request message by client rpc.
  /// @param expire_time_ms How long to expire this request, in millisecond.
  /// @return true: success, false: failed.
  bool Push(uint32_t unique_id, CTransportReqMsg* msg, size_t expire_time_ms);

  /// @brief Get request message then delete it from timeout queue by request id.
  /// @param unique_id Unique id for request message.
  /// @return Pointer to request message, nullptr if not found.
  CTransportReqMsg* Pop(uint32_t unique_id);

  /// @brief Get request message then delete it from timeout queue.
  /// @return Pointer to request message, nullptr if queue is empty.
  CTransportReqMsg* Pop();

  /// @brief Pop request message from timeout queue by DataIterator.
  /// @param iter Iterator of request message in hashmap.
  /// @return Pointer to request message.
  /// @note Used by timeout_handler to get request message.
  CTransportReqMsg* GetAndPop(const DataIterator& iter);

  /// @brief Check timeout, and run timeout handler if neccesary.
  /// @param now_ms Current time in millisecond.
  /// @param timeout_handler Handle funtion for request timeout.
  /// @return true: at least one request timeout handling, false: no request timeout handling.
  bool DoTimeout(size_t now_ms, const Function<void(const DataIterator&)>& timeout_handler);

  /// @brief Check request message labeled by id exist in timeout queue or not.
  /// @param unique_id Unique id for request message.
  /// @return true: exist, false: not exist.
  bool Contains(uint32_t unique_id) { return hash_map_.find(unique_id) != hash_map_.end(); }

  /// @brief Get request message from timeout queue by unique id, but not delete it
  /// @param unique_id Unique id for request message.
  /// @return Pointer to request message, nullptr if not found.
  CTransportReqMsg* Get(uint32_t unique_id);

  /// @brief Get how many requests in timeout queue.
  size_t Size() const { return hash_map_.size(); }

 private:
  // Map from request id to request message.
  T hash_map_;

  // Timing wheel to implement timeout.
  TimingWheel<T> timing_wheel_;
};

template <typename T, typename V>
bool TimingWheelTimeoutQueueImpl<T, V>::Push(uint32_t unique_id, CTransportReqMsg* msg, size_t timeout) {
  V value;
  value.msg = msg;

  std::pair<DataIterator, bool> result = hash_map_.insert(std::make_pair(unique_id, value));
  if (result.second) {
    result.first->second.time_node = timing_wheel_.Add(timeout, result.first);
  }

  return result.second;
}

template <typename T, typename V>
bool TimingWheelTimeoutQueueImpl<T, V>::DoTimeout(size_t now_ms,
                                                      const Function<void(const DataIterator&)>& timeout_handle) {
  return timing_wheel_.DoTimeout(now_ms, timeout_handle);
}

template <typename T, typename V>
CTransportReqMsg* TimingWheelTimeoutQueueImpl<T, V>::Pop(uint32_t unique_id) {
  DataIterator it = hash_map_.find(unique_id);

  if (it == hash_map_.end()) {
    return nullptr;
  }

  CTransportReqMsg* msg = it->second.msg;

  timing_wheel_.Delete(it->second.time_node);
  hash_map_.erase(it);

  return msg;
}

template <typename T, typename V>
CTransportReqMsg* TimingWheelTimeoutQueueImpl<T, V>::Pop() {
  if (hash_map_.empty()) {
    return nullptr;
  }

  DataIterator it = hash_map_.begin();
  CTransportReqMsg* msg = it->second.msg;
  timing_wheel_.Delete(it->second.time_node);
  hash_map_.erase(it);

  return msg;
}

template <typename T, typename V>
CTransportReqMsg* TimingWheelTimeoutQueueImpl<T, V>::GetAndPop(const DataIterator& iter) {
  CTransportReqMsg* msg = iter->second.msg;
  hash_map_.erase(iter);
  return msg;
}

template <typename T, typename V>
CTransportReqMsg* TimingWheelTimeoutQueueImpl<T, V>::Get(uint32_t unique_id) {
  DataIterator it = hash_map_.find(unique_id);
  if (it == hash_map_.end()) {
    return nullptr;
  }

  return it->second.msg;
}

using TimingWheelTimeoutQueue = TimingWheelTimeoutQueueImpl<HashMap, HashMapValue>;
using SmallCacheTimingWheelTimeoutQueue = TimingWheelTimeoutQueueImpl<SmallCacheHashMap, SmallCacheHashMapValue>;

}  // namespace trpc::internal

namespace trpc::object_pool {

template <>
struct ObjectPoolTraits<trpc::internal::TimerNode<trpc::internal::HashMap>> {
#if defined(TRPC_DISABLED_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kDisabled;
#elif defined(TRPC_SHARED_NOTHING_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
#else
  static constexpr auto kType = ObjectPoolType::kGlobal;
#endif
};

template <>
struct ObjectPoolTraits<trpc::internal::TimerNode<trpc::internal::SmallCacheHashMap>> {
#if defined(TRPC_DISABLED_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kDisabled;
#elif defined(TRPC_SHARED_NOTHING_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
#else
  static constexpr auto kType = ObjectPoolType::kGlobal;
#endif
};

}  // namespace trpc::object_pool
