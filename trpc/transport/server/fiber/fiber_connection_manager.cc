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

#include "trpc/transport/server/fiber/fiber_connection_manager.h"

#include "trpc/util/hash_util.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc {

constexpr static size_t kReserveSize = 1024 * 8 - 1;

FiberConnectionManager::FiberConnectionManager() {
  conn_shards_ = std::make_unique<ConnectionShard[]>(kShards);
  for (size_t i = 0; i != kShards; ++i) {
    auto&& shard = conn_shards_[i];
    shard.map.reserve(kReserveSize);
  }
}

FiberConnectionManager::~FiberConnectionManager() {
  for (size_t i = 0; i != kShards; ++i) {
    auto&& shard = conn_shards_[i];
    TRPC_ASSERT(shard.map.empty());
  }
}

void FiberConnectionManager::Add(uint64_t conn_id, const RefPtr<FiberTcpConnection>& conn) {
  auto&& shard = conn_shards_[GetHashIndex(conn_id, kShards)];

  std::scoped_lock _(shard.lock);
  auto&& [it, inserted] = shard.map.insert(std::make_pair(conn_id, conn));
  (void)it;  // Suppresses compilation warnings.

  TRPC_ASSERT(inserted && "insert FiberConnectionManager with Duplicate conn_id");
}

RefPtr<FiberTcpConnection> FiberConnectionManager::Del(uint64_t conn_id) {
  auto&& shard = conn_shards_[GetHashIndex(conn_id, kShards)];

  std::scoped_lock _(shard.lock);
  auto it = shard.map.find(conn_id);
  if (TRPC_LIKELY(it != shard.map.end())) {
    auto v = std::move(it->second);
    shard.map.erase(it);
    return v;
  }

  return nullptr;
}

RefPtr<FiberTcpConnection> FiberConnectionManager::Get(uint64_t conn_id) {
  auto&& shard = conn_shards_[GetHashIndex(conn_id, kShards)];

  std::scoped_lock _(shard.lock);
  auto it = shard.map.find(conn_id);
  if (TRPC_LIKELY(it != shard.map.end())) {
    return it->second;
  }

  return nullptr;
}

void FiberConnectionManager::GetIdles(uint32_t idle_timeout, std::vector<RefPtr<FiberTcpConnection>>& idle_conns) {
  TRPC_ASSERT(conn_shards_ && "conn_shards_ is null");
  uint64_t current_time = trpc::time::GetMilliSeconds();

  for (size_t i = 0; i < kShards; ++i) {
    auto&& shard = conn_shards_[i];

    std::scoped_lock _(shard.lock);
    auto it = shard.map.begin();
    while (it != shard.map.end()) {
      if (it->second->GetConnActiveTime() + idle_timeout < current_time) {
        idle_conns.emplace_back(std::move(it->second));
        it = shard.map.erase(it);
      } else {
        ++it;
      }
    }
  }
}

void FiberConnectionManager::DisableAllConnRead() {
  for (size_t i = 0; i != kShards; ++i) {
    auto&& shard = conn_shards_[i];

    std::scoped_lock _(shard.lock);
    auto it = shard.map.begin();
    while (it != shard.map.end()) {
      it->second->DisableRead();
      ++it;
    }
  }
}

void FiberConnectionManager::Stop() {
  for (size_t i = 0; i != kShards; ++i) {
    auto&& shard = conn_shards_[i];

    std::unordered_map<uint64_t, RefPtr<FiberTcpConnection>> temp;
    {
      std::scoped_lock _(shard.lock);
      shard.map.swap(temp);
    }
    auto it = temp.begin();
    while (it != temp.end()) {
      it->second->GetConnectionHandler()->Stop();
      it->second->GetConnectionHandler()->Join();
      it->second->Stop();
      it->second->Join();

      it++;
    }
  }
}

void FiberConnectionManager::Destroy() {
  for (size_t i = 0; i != kShards; ++i) {
    auto&& shard = conn_shards_[i];

    std::unordered_map<uint64_t, RefPtr<FiberTcpConnection>> temp;
    {
      std::scoped_lock _(shard.lock);
      shard.map.swap(temp);
    }
    auto it = temp.begin();
    while (it != temp.end()) {
      it = temp.erase(it);
    }

    shard.map.clear();
  }
}

}  // namespace trpc
