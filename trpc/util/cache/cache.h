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

#include <functional>
#include <optional>

namespace trpc::cache {

/// @brief Abstract base class for cache implementations.
///
/// @tparam KeyType   Type of the cache keys.
/// @tparam ValueType Type of the cache values.
/// @tparam HashFn    Hash function for keys (default: std::hash<KeyType>).
/// @tparam KeyEqual  Key equality comparator (default: std::equal_to<KeyType>).
/// @tparam Mutex     Mutex type for synchronization (default: std::mutex).
template <typename KeyType, typename ValueType, typename HashFn = std::hash<KeyType>,
          typename KeyEqual = std::equal_to<KeyType>>
class Cache {
 public:
  Cache() = default;
  virtual ~Cache() = default;

  /// @brief Insert or update a key-value pair into the cache (copy semantics).
  /// @param key The key to insert/update.
  /// @param value The value to associate with the key.
  /// @return true if insertion succeeded, false otherwise (e.g., key exists depending on impl).
  virtual bool Put(const KeyType& key, const ValueType& value) = 0;

  /// @brief Insert or update a key-value pair into the cache (move semantics).
  /// @param  key The key to insert/update.
  /// @param value The value to associate with the key (will be moved).
  /// @return  true if insertion succeeded, false otherwise (e.g., key exists depending on impl).
  virtual bool Put(const KeyType& key, ValueType&& value) = 0;

  /// @brief Retrieves the value associated with the given key.
  /// @param key The key to look up.
  /// @return An optional containing the value if the key exists, std::nullopt otherwise.
  virtual std::optional<ValueType> Get(const KeyType& key) = 0;

  /// @brief Removes the key-value pair associated with the given key.
  /// @param key The key to remove.
  /// @return true if the key was found and removed, false otherwise.
  virtual bool Remove(const KeyType& key) = 0;

  /// @brief Clear all key-value pairs from the cache.
  virtual void Clear() = 0;

  /// @brief Get the number of entries currently in the cache.
  /// @return The current size of the cache.
  virtual size_t Size() = 0;
};

}  // namespace trpc::cache
