/*
 *
 * Tencent is pleased to support the open source community by making
 * tRPC available.
 *
 * Copyright (C) 2025 Tencent.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#pragma once

#include <atomic>
#include <chrono>
#include <list>
#include <memory>
#include <mutex>

#include "trpc/util/cache/cache.h"

namespace trpc::cache {

/// @brief A TTL (Time-To-Live) Cache implementation that automatically evicts entries after expiration.
///        This class decorates another Cache implementation and adds TTL expiration logic.
///        Uses sharded mutexes for concurrent access and lazy expiration cleanup.
///
/// @tparam KeyType   Type of the cache keys.
/// @tparam ValueType Type of the cache values.
/// @tparam HashFn    Hash function for keys (default: std::hash<KeyType>).
/// @tparam KeyEqual  Key equality comparator (default: std::equal_to<KeyType>).
/// @tparam Mutex     Mutex type for synchronization (default: std::mutex).
template <typename KeyType, typename ValueType, typename HashFn = std::hash<KeyType>,
          typename KeyEqual = std::equal_to<KeyType>, typename Mutex = std::mutex>
class ScheduledCache final : public Cache<KeyType, ValueType, HashFn, KeyEqual> {
 public:
  /// @brief Constructs a TTL Cache with a wrapped cache instance and number of shards.
  /// @param cache      The underlying cache implementation to decorate.
  /// @param default_ttl Default TTL duration for entries in milliseconds (default: 60000ms).
  /// @param shards_num Number of shards for concurrent access (default: 32).
  explicit ScheduledCache(std::unique_ptr<Cache<KeyType, ValueType, HashFn, KeyEqual>> cache,
                          std::chrono::milliseconds default_ttl = std::chrono::milliseconds(60000),
                          size_t shards_num = 32)
      : default_ttl_(default_ttl), cache_(std::move(cache)) {
    shards_num_ = RoundUpPowerOf2(shards_num);
    shards_ = std::make_unique<Shard<KeyType>[]>(shards_num_);
  }

  ~ScheduledCache() = default;

  // Disable copying
  ScheduledCache(const ScheduledCache&) = delete;
  ScheduledCache& operator=(const ScheduledCache&) = delete;

  // Disable moving
  ScheduledCache(ScheduledCache&& other) = delete;
  ScheduledCache& operator=(ScheduledCache&& other) = delete;

  /// @brief Insert or update a key-value pair with default TTL (copy semantics).
  /// @param key The key to insert or update.
  /// @param value The value to associate with the key.
  /// @return true if insertion succeeded, false otherwise.
  bool Put(const KeyType& key, const ValueType& value) override { return Put(key, value, default_ttl_); }

  /// @brief Insert or update a key-value pair with default TTL (move semantics).
  /// @param key The key to insert or update.
  /// @param value The value to associate with the key (will be moved).
  /// @return true if insertion succeeded, false otherwise.
  bool Put(const KeyType& key, ValueType&& value) override {
    return Put(key, std::forward<ValueType>(value), default_ttl_);
  }

  /// @brief Insert or update a key-value pair with custom TTL (copy semantics).
  /// @param key The key to insert or update.
  /// @param value The value to associate with the key.
  /// @param ttl Custom TTL duration for this entry.
  /// @return true if insertion succeeded, false otherwise.
  bool Put(const KeyType& key, const ValueType& value, std::chrono::milliseconds ttl) {
    size_t hash_code = HashFn()(key);
    size_t idx = GetShardIdx(hash_code);
    auto& shard = shards_[idx];

    auto expire_time = std::chrono::steady_clock::now() + ttl;
    {
      std::lock_guard<Mutex> lock(shard.mutex);

      if (auto it = shard.expiry_list.find(key); it != shard.expiry_list.end()) {
        if (it->second <= std::chrono::steady_clock::now()) {
          shard.expiry_list.erase(it);
          cache_->Remove(key);
        }
      } else {
        entries_num.fetch_add(1, std::memory_order_relaxed);
      }
      shard.expiry_list[key] = expire_time;
    }

    return cache_->Put(key, value);
  }

  /// @brief Insert or update a key-value pair with custom TTL (move semantics).
  /// @param key The key to insert or update.
  /// @param value The value to associate with the key.
  /// @param ttl Custom TTL duration for this entry.
  /// @return true if insertion succeeded, false otherwise.
  bool Put(const KeyType& key, ValueType&& value, std::chrono::milliseconds ttl) {
    size_t hash_code = HashFn()(key);
    size_t idx = GetShardIdx(hash_code);
    auto& shard = shards_[idx];

    auto expire_time = std::chrono::steady_clock::now() + ttl;
    {
      std::lock_guard<Mutex> lock(shard.mutex);

      if (auto it = shard.expiry_list.find(key); it != shard.expiry_list.end()) {
        if (it->second <= std::chrono::steady_clock::now()) {
          shard.expiry_list.erase(it);
          cache_->Remove(key);
        }
      } else {
        entries_num.fetch_add(1, std::memory_order_relaxed);
      }
      shard.expiry_list[key] = expire_time;
    }

    return cache_->Put(key, std::forward<ValueType>(value));
  }

  /// @brief Retrieves the value if the key exists and hasn't expired.
  /// @param key The key to look up.
  /// @return An optional containing the value if the key exists and is valid, std::nullopt otherwise.
  std::optional<ValueType> Get(const KeyType& key) override {
    size_t hash_code = HashFn()(key);
    size_t idx = GetShardIdx(hash_code);
    auto& shard = shards_[idx];

    {
      std::lock_guard<Mutex> lock(shard.mutex);

      if (auto it = shard.expiry_list.find(key); it != shard.expiry_list.end()) {
        if (it->second > std::chrono::steady_clock::now()) {
          return cache_->Get(key);
        }
        // Key has expired, remove it (lazy evaluation)
        shard.expiry_list.erase(it);
        cache_->Remove(key);
        entries_num.fetch_sub(1, std::memory_order_relaxed);
      }
    }

    return std::nullopt;
  }

  /// @brief Removes the key-value pair if it exists.
  /// @param key The key to remove.
  /// @return true if the key was found and removed, false otherwise.
  bool Remove(const KeyType& key) override {
    size_t hash_code = HashFn()(key);
    size_t idx = GetShardIdx(hash_code);
    auto& shard = shards_[idx];

    {
      std::lock_guard<Mutex> lock(shard.mutex);

      if (auto it = shard.expiry_list.find(key); it == shard.expiry_list.end()) {
        return false;
      }

      shard.expiry_list.erase(key);
    }
    entries_num.fetch_sub(1, std::memory_order_relaxed);

    return cache_->Remove(key);
  }

  /// @brief Clear all entries from the cache.
  void Clear() override {
    for (size_t i = 0; i < shards_num_; ++i) {
      auto& shard = shards_[i];
      std::lock_guard<Mutex> lock(shard.mutex);
      shard.expiry_list.clear();
    }
    entries_num.store(0, std::memory_order_relaxed);
    cache_->Clear();
  }

  /// @brief Get the number of entries currently in the cache.
  /// @return The current size of the cache.
  size_t Size() override { return entries_num.load(std::memory_order_acquire); }

 private:
  template <typename K, typename M = std::mutex>
  struct alignas(64) Shard {
    M mutex;
    std::unordered_map<K, std::chrono::steady_clock::time_point> expiry_list;

    Shard() = default;
    ~Shard() = default;

    // Disable copying
    Shard(const Shard&) = delete;
    Shard& operator=(const Shard&) = delete;

    // Disable moving
    Shard(Shard&&) = delete;
    Shard& operator=(Shard&&) = delete;
  };

  size_t GetShardIdx(size_t hash_code) const { return hash_code & (shards_num_ - 1); }

  size_t RoundUpPowerOf2(size_t n) {
    bool size_is_power_of_2 = (n >= 1) && ((n & (n - 1)) == 0);
    if (!size_is_power_of_2) {
      uint64_t tmp = 1;
      while (tmp <= n) {
        tmp <<= 1;
      }

      n = tmp;
    }
    return n;
  }

 private:
  /// Default TTL duration for entries in milliseconds.
  std::chrono::milliseconds default_ttl_;
  // The underlying cache implementation.
  std::unique_ptr<Cache<KeyType, ValueType, HashFn, KeyEqual>> cache_;
  // Number of shards (always power of two).
  size_t shards_num_;
  // Atomic counter for the total number of entries.
  std::atomic<size_t> entries_num{0};
  // Array of shards.
  std::unique_ptr<Shard<KeyType>[]> shards_;
};

}  // namespace trpc::cache