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

#include "trpc/util/cache/cache.h"
#include "trpc/util/concurrency/lightly_concurrent_hashmap.h"

namespace trpc::cache {

/// @brief Basic thread-safe cache implementation using LightlyConcurrentHashMap.
///
/// @tparam KeyType   Type of the cache keys.
/// @tparam ValueType Type of the cache values.
/// @tparam HashFn    Hash function for keys (default: std::hash<KeyType>).
/// @tparam KeyEqual  Key equality comparator (default: std::equal_to<KeyType>).
template <typename KeyType, typename ValueType, typename HashFn = std::hash<KeyType>,
          typename KeyEqual = std::equal_to<KeyType>>
class BasicCache final : public Cache<KeyType, ValueType, HashFn, KeyEqual> {
 public:
  BasicCache() = default;
  ~BasicCache() = default;

  BasicCache(const BasicCache&) = delete;
  BasicCache& operator=(const BasicCache&) = delete;

  BasicCache(BasicCache&& other) noexcept = default;
  BasicCache& operator=(BasicCache&& other) noexcept = default;

  /// @brief Insert or update a key-value pair into the cache (copy semantics).
  /// @param key The key to insert or update.
  /// @param value The value to associate with the key.
  /// @return  true if insertion succeeded, false otherwise (e.g., key exists).
  bool Put(const KeyType& key, const ValueType& value) override { return cache_map_.InsertOrAssign(key, value); }

  /// @brief Insert or update a key-value pair into the cache (move semantics).
  /// @param key The key to insert or update.
  /// @param value The value to associate with the key.
  /// @return  true if insertion succeeded, false otherwise (e.g., key exists).
  bool Put(const KeyType& key, ValueType&& value) override {
    return cache_map_.InsertOrAssign(key, std::forward<ValueType>(value));
  }

  /// @brief Retrieves the value associated with the given key.
  /// @param key The key to look up.
  /// @return An optional containing the value if the key exists, std::nullopt otherwise.
  std::optional<ValueType> Get(const KeyType& key) override {
    ValueType value;

    if (cache_map_.Get(key, value)) {
      return value;
    }

    return std::nullopt;
  }

  /// @brief Removes the key-value pair associated with the given key.
  /// @param key The key to remove.
  /// @return true if the key was found and removed, false otherwise.
  bool Remove(const KeyType& key) override { return cache_map_.Erase(key); }

  /// @brief Clear all key-value pairs from the cache.
  void Clear() override { cache_map_.Clear(); }

  /// @brief Get the number of entries currently in the cache.
  /// @return The current size of the cache.
  size_t Size() override { return cache_map_.Size(); }

 private:
  /// @brief The underlying concurrent hash map that stores the actual key-value data.
  concurrency::LightlyConcurrentHashMap<KeyType, ValueType, HashFn, KeyEqual> cache_map_;
};

}  // namespace trpc::cache
