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

#include "trpc/util/cache/decorators/lru_cache.h"
#include "trpc/util/cache/impl/basic_cache.h"

#include "gtest/gtest.h"

namespace trpc::cache::testing {

/// @brief Test basic Put/Get operations and size tracking.
/// @note Verifies fundamental cache operations including insertion, retrieval,
///       and size maintenance functionality.
TEST(LRUCacheTest, BasicPutGetSize) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  LRUCache<int, int> lru_cache(std::move(basic_cache));

  ASSERT_EQ(lru_cache.Size(), 0);
  ASSERT_TRUE(lru_cache.Put(1, 1));
  ASSERT_EQ(lru_cache.Get(1), 1);
  ASSERT_EQ(lru_cache.Size(), 1);
}

/// @brief Test handling of duplicate key insertion.
/// @note Verifies that inserting existing keys updates values correctly
///       without affecting cache size.
TEST(LRUCacheTest, PutDuplicateKey) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  LRUCache<int, int> lru_cache(std::move(basic_cache));

  ASSERT_TRUE(lru_cache.Put(1, 1));
  ASSERT_FALSE(lru_cache.Put(1, 2));
  ASSERT_EQ(lru_cache.Size(), 1);
  ASSERT_EQ(lru_cache.Get(1), 2);
}

/// @brief Test removal of existing keys.
/// @note Verifies successful removal of existing entries and subsequent cache state.
TEST(LRUCacheTest, RemoveExistingKey) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  LRUCache<int, int> lru_cache(std::move(basic_cache));

  lru_cache.Put(1, 1);
  ASSERT_TRUE(lru_cache.Remove(1));
  ASSERT_EQ(lru_cache.Get(1), std::nullopt);
  ASSERT_EQ(lru_cache.Size(), 0);
}

/// @brief Test removal of non-existent keys.
/// @note Verifies cache behavior when attempting to remove non-existing keys.
TEST(LRUCacheTest, RemoveNonExistingKey) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  LRUCache<int, int> lru_cache(std::move(basic_cache));

  ASSERT_FALSE(lru_cache.Remove(1));
  ASSERT_EQ(lru_cache.Size(), 0);
}

/// @brief Test complete cache clearance.
/// @note Verifies that Clear() operation removes all entries and resets cache state.
TEST(LRUCacheTest, ClearCache) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  LRUCache<int, int> lru_cache(std::move(basic_cache));

  lru_cache.Put(1, 1);
  lru_cache.Put(2, 2);
  lru_cache.Clear();

  ASSERT_EQ(lru_cache.Size(), 0);
  ASSERT_EQ(lru_cache.Get(1), std::nullopt);
  ASSERT_EQ(lru_cache.Get(2), std::nullopt);
}

/// @brief Test LRU eviction policy when capacity is exceeded.
/// @note Verifies that least recently used entries are evicted according to LRU policy when cache reaches capacity.
TEST(LRUCacheTest, EvictLeastRecentlyUsedKeyWhenFull) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  LRUCache<int, int> lru_cache(std::move(basic_cache), 3, 1);

  // Insert 3 entries
  lru_cache.Put(1, 1);
  lru_cache.Put(2, 2);
  lru_cache.Put(3, 3);

  // Access key 1 to make it most recently used
  ASSERT_EQ(lru_cache.Get(1), 1);

  // Insert 4th item - should evict least recently used (key 2)
  lru_cache.Put(4, 4);

  // Verify eviction
  ASSERT_EQ(lru_cache.Get(2), std::nullopt);  // Least recently used (key 2) should be evicted
  ASSERT_EQ(lru_cache.Get(1), 1);             // Most recently used (key 1) should remain
  ASSERT_EQ(lru_cache.Get(3), 3);             // Middle used (key 3) should remain
  ASSERT_EQ(lru_cache.Get(4), 4);             // Newly added (key 4) should remain
  ASSERT_EQ(lru_cache.Size(), 3);
}

/// @brief Test concurrent Put operations under high contention.
/// @note Verifies that concurrent operations maintain data consistency.
TEST(LRUCacheTest, ConcurrentPut) {
  constexpr int capacity = 1000;
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  LRUCache<int, int> lru_cache(std::move(basic_cache), capacity, 32);

  constexpr int threads_num = 8;
  constexpr int puts_per_thread = 200;
  constexpr int puts_num = threads_num * puts_per_thread;
  std::vector<std::thread> threads;
  std::atomic<int> key_gen{0};

  for (int i = 0; i < threads_num; ++i) {
    threads.emplace_back([&]() {
      for (int j = 0; j < puts_per_thread; ++j) {
        int key = key_gen.fetch_add(1, std::memory_order_relaxed);
        lru_cache.Put(key, key);
      }
    });
  }
  for (auto& t : threads) t.join();

  int hits{0}, misses{0};

  for (int key = 0; key < puts_num; ++key) {
    if (lru_cache.Get(key) == std::nullopt) {
      ++misses;
    } else {
      ++hits;
    }
  }

  ASSERT_EQ(hits, capacity);
  ASSERT_EQ(misses, puts_num - capacity);
}

/// @brief Test concurrent Get operations under high contention.
/// @note Verifies that concurrent operations maintain data consistency.
TEST(LRUCacheTest, ConcurrentGet) {
  const int capacity = 1000;
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  LRUCache<int, int> lru_cache(std::move(basic_cache), capacity);

  for (int key = 0; key < capacity; ++key) {
    lru_cache.Put(key, key);
  }

  constexpr int threads_num = 8;
  constexpr int gets_per_thread = 200;
  std::vector<std::thread> threads;

  for (int i = 0; i < threads_num; ++i) {
    threads.emplace_back([&, i]() {
      for (int j = 0; j < gets_per_thread; ++j) {
        int key = j + i * gets_per_thread;
        if (key < capacity) {
          ASSERT_EQ(lru_cache.Get(key), key);
        } else {
          ASSERT_EQ(lru_cache.Get(key), std::nullopt);
        }
      }
    });
  }
  for (auto& t : threads) t.join();
}

}  // namespace trpc::cache::testing
