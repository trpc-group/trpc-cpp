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

#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/transport/client/fiber/common/call_context.h"
#include "trpc/util/align.h"
#include "trpc/util/hash_util.h"
#include "trpc/util/likely.h"

namespace trpc {

constexpr int16_t kCallMapShards = 128;

/// @brief Multi-thread safe key/value operation template class
template <class T>
class ShardedCallMap {
 public:
  ShardedCallMap() { shards_ = std::make_unique<Shard[]>(kCallMapShards); }

  /// @brief Insert the corresponding value according to the key
  /// @param correlation_id The corresponding key is not directly used as a key,
  /// it uses to be converted by GetHashIndex
  /// @param value Insert value
  void Insert(std::uint64_t correlation_id, T value) {
    auto&& shard = shards_[GetHashIndex(correlation_id, kCallMapShards)];
    std::scoped_lock _(shard.lock);

    auto&& [iter, inserted] = shard.map.emplace(correlation_id, std::move(value));
    (void)iter;  // eliminate warnings
    TRPC_ASSERT(inserted && "insert ShardedCallMap with Duplicate correlation_id");
  }

  /// @brief Delete the corresponding object according to the key
  /// @param correlation_id The corresponding key is not directly used as a key,
  /// it uses to be converted by GetHashIndex
  /// @return Return the value to delete
  /// @note Returns nullptr if the corresponding key/value not found
  T Remove(std::uint64_t correlation_id) {
    auto&& shard = shards_[GetHashIndex(correlation_id, kCallMapShards)];
    std::scoped_lock _(shard.lock);

    if (auto iter = shard.map.find(correlation_id); TRPC_LIKELY(iter != shard.map.end())) {
      auto v = std::move(iter->second);
      shard.map.erase(iter);
      return v;
    }
    return nullptr;
  }

  /// @brief Traverse and process the stored key/value
  /// @param f Handle function
  template <class F>
  void ForEach(F&& f) {
    for (int i = 0; i != kCallMapShards; ++i) {
      auto&& shard = shards_[i];
      std::scoped_lock _(shard.lock);

      for (auto&& [k, v] : shard.map) {
        std::forward<F>(f)(k, v);
      }
    }
  }

  /// @brief Clear all elements
  void Clear() {
    for (int i = 0; i != kCallMapShards; ++i) {
      auto&& shard = shards_[i];
      std::scoped_lock _(shard.lock);

      shard.map.clear();
    }
  }

 private:
  struct alignas(hardware_destructive_interference_size) Shard {
    std::mutex lock;
    std::unordered_map<std::uint64_t, T> map;
  };
  std::unique_ptr<Shard[]> shards_;
};

/// @brief Map for request id/context
class CallMap : public RefCounted<CallMap> {
 public:
  std::pair<CallContext*, std::unique_lock<Spinlock>> AllocateContext(uint32_t correlation_id) {
    auto ptr = object_pool::MakeLwUnique<CallContext>();
    auto result = std::pair(ptr.Get(), std::unique_lock(ptr->lock));
    ctxs_.Insert(static_cast<uint64_t>(correlation_id), std::move(ptr));
    return result;
  }

  object_pool::LwUniquePtr<CallContext> TryReclaimContext(uint32_t correlation_id) {
    return ctxs_.Remove(correlation_id);
  }

  template <class F>
  void ForEach(F&& f) {
    ctxs_.ForEach([&](auto&& k, auto&& v) { f(k, v.Get()); });
  }

 private:
  ShardedCallMap<object_pool::LwUniquePtr<CallContext>> ctxs_;
};

}  // namespace trpc
