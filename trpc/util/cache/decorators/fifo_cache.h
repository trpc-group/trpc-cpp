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
#include <memory>
#include <mutex>
#include <queue>

#include "trpc/util/cache/cache.h"

namespace trpc::cache {

/// @brief A FIFO (First-In-First-Out) Cache implementation that evicts the oldest entry upon exceeding capacity.
///        This class decorates another Cache implementation (e.g., BasicCache) and adds FIFO eviction logic.
///        Thread-safety is achieved using a mutex to guard all operations.
///
/// @tparam KeyType   Type of the cache keys.
/// @tparam ValueType Type of the cache values.
/// @tparam HashFn    Hash function for keys (default: std::hash<KeyType>).
/// @tparam KeyEqual  Key equality comparator (default: std::equal_to<KeyType>).
/// @tparam Mutex     Mutex type for synchronization (default: std::mutex).
template <typename KeyType, typename ValueType, typename HashFn = std::hash<KeyType>,
          typename KeyEqual = std::equal_to<KeyType>, typename Mutex = std::mutex>
class FIFOCache final : public Cache<KeyType, ValueType, HashFn, KeyEqual> {
 public:
  /// @brief Constructs a FIFO Cache with a wrapped cache instance, maximum capacity, and number of shards.
  /// @param cache     The underlying cache implementation to decorate.
  /// @param capacity  Maximum number of entries allowed in the cache (default: 1024).
  /// @param shards_num Number of shards for concurrent access (default: 32).
  explicit FIFOCache(std::unique_ptr<Cache<KeyType, ValueType, HashFn, KeyEqual>> cache, size_t capacity = 1024,
                     size_t shards_num = 32)
      : capacity_(capacity), cache_(std::move(cache)) {
    shards_num_ = RoundUpPowerOf2(shards_num);
    shards_ = std::make_unique<Shard<KeyType>[]>(shards_num_);
    // Calculate capacity per shard, ensuring at least 1 per shard
    capacity_per_shard_ = (capacity_ + shards_num_ - 1) / shards_num_;
  }

  ~FIFOCache() = default;

  // Disable copying
  FIFOCache(const FIFOCache&) = delete;
  FIFOCache& operator=(const FIFOCache&) = delete;

  // Disable moving
  FIFOCache(FIFOCache&& other) = delete;
  FIFOCache& operator=(FIFOCache&& other) = delete;

  /// @brief Insert or update a key-value pair into the cache (copy semantics).
  /// @param key The key to insert or update.
  /// @param value The value to associate with the key.
  /// @return true if insertion succeeded, false otherwise (e.g., key exists).
  bool Put(const KeyType& key, const ValueType& value) override {
    size_t hash_code = HashFn()(key);
    size_t idx = GetShardIdx(hash_code);
    auto& shard = shards_[idx];

    {
      std::lock_guard<Mutex> lock(shard.mutex);

      // If key exists, update value in underlying cache but don't change FIFO order
      if (auto it = shard.keys.find(key); it != shard.keys.end()) {
        return cache_->Put(key, value);
      }

      if (entries_num >= capacity_) {
        // If shard is full, evict the oldest key (front of the queue)
        const KeyType& oldest_key = shard.fifo_queue.front();
        shard.keys.erase(oldest_key);
        cache_->Remove(oldest_key);
        shard.fifo_queue.pop();
      } else {
        // Increase key count for new entry
        entries_num.fetch_add(1, std::memory_order_relaxed);
      }

      // Add new key to the back of the queue
      shard.fifo_queue.push(key);
      // Just mark existence, no iterator needed for FIFO
      shard.keys[key] = true;
    }

    return cache_->Put(key, value);
  }

  /// @brief Insert or update a key-value pair into the cache (move semantics).
  /// @param key The key to insert or update.
  /// @param value The value to associate with the key.
  /// @return true if insertion succeeded, false otherwise (e.g., key exists).
  bool Put(const KeyType& key, ValueType&& value) override {
    size_t hash_code = HashFn()(key);
    size_t idx = GetShardIdx(hash_code);
    auto& shard = shards_[idx];

    {
      std::lock_guard<Mutex> lock(shard.mutex);

      // If key exists, update value in underlying cache but don't change FIFO order
      if (auto it = shard.keys.find(key); it != shard.keys.end()) {
        return cache_->Put(key, std::forward<ValueType>(value));
      }
      if (size_t size = entries_num.load(std::memory_order_acquire); size >= capacity_) {
        // If shard is full, evict the oldest key (front of the queue)
        const KeyType& oldest_key = shard.fifo_queue.front();
        shard.keys.erase(oldest_key);
        cache_->Remove(oldest_key);
        shard.fifo_queue.pop();
      } else {
        // Increase key count for new entry
        entries_num.fetch_add(1, std::memory_order_relaxed);
      }

      // Add new key to the back of the queue
      shard.fifo_queue.push(key);
      // Just mark existence, no iterator needed for FIFO
      shard.keys[key] = true;
    }

    return cache_->Put(key, std::forward<ValueType>(value));
  }

  /// @brief Retrieves the value associated with the given key.
  /// @param key The key to look up.
  /// @return An optional containing the value if the key exists, std::nullopt otherwise.
  std::optional<ValueType> Get(const KeyType& key) override {
    // For FIFO, getting a value doesn't change its position in the queue
    return cache_->Get(key);
  }

  /// @brief Removes the key-value pair associated with the given key.
  /// @param key The key to remove.
  /// @return true if the key was found and removed, false otherwise.
  bool Remove(const KeyType& key) override {
    size_t hash_code = HashFn()(key);
    size_t idx = GetShardIdx(hash_code);
    auto& shard = shards_[idx];

    {
      std::lock_guard<Mutex> lock(shard.mutex);

      if (auto it = shard.keys.find(key); it == shard.keys.end()) {
        return false;
      }

      shard.keys.erase(key);
    }
    entries_num.fetch_sub(1, std::memory_order_relaxed);

    return cache_->Remove(key);
  }

  /// @brief Clear all key-value pairs from the cache.
  void Clear() override {
    for (size_t i = 0; i < shards_num_; ++i) {
      auto& shard = shards_[i];
      std::lock_guard<Mutex> lock(shard.mutex);
      // Clear FIFO queue and key map
      while (!shard.fifo_queue.empty()) {
        shard.fifo_queue.pop();
      }
      shard.keys.clear();
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
    std::queue<K> fifo_queue;
    std::unordered_map<K, bool> keys;

    Shard() = default;
    ~Shard() = default;

    // Disable copying
    Shard(const Shard&) = delete;
    Shard& operator=(const Shard&) = delete;

    // Disable  moving
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
  // Maximum number of entries allowed.
  size_t capacity_;
  // The underlying cache implementation.
  std::unique_ptr<Cache<KeyType, ValueType, HashFn, KeyEqual>> cache_;
  // Number of shards (always power of two).
  size_t shards_num_;
  // Capacity per individual shard.
  size_t capacity_per_shard_;
  // Atomic counter for the total number of entries.
  std::atomic<size_t> entries_num{0};
  // Array of shards
  std::unique_ptr<Shard<KeyType>[]> shards_;
};

}  // namespace trpc::cache
