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

#include "trpc/util/cache/decorators/fifo_cache.h"
#include "trpc/util/cache/impl/basic_cache.h"

#include "gtest/gtest.h"

namespace trpc::cache::testing {

/// @brief Test basic Put/Get operations and size tracking.
/// @note Verifies fundamental cache operations including insertion, retrieval,
///       and size maintenance functionality.
TEST(FIFOCacheTest, BasicPutGetSize) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  FIFOCache<int, int> fifo_cache(std::move(basic_cache));

  ASSERT_EQ(fifo_cache.Size(), 0);
  ASSERT_TRUE(fifo_cache.Put(1, 1));
  ASSERT_EQ(fifo_cache.Get(1), 1);
  ASSERT_EQ(fifo_cache.Size(), 1);
}

/// @brief Test handling of duplicate key insertion.
/// @note Verifies that inserting existing keys updates values correctly
///       without affecting cache size.
TEST(FIFOCacheTest, PutDuplicateKey) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  FIFOCache<int, int> fifo_cache(std::move(basic_cache));

  ASSERT_TRUE(fifo_cache.Put(1, 1));
  ASSERT_FALSE(fifo_cache.Put(1, 2));
  ASSERT_EQ(fifo_cache.Size(), 1);
  ASSERT_EQ(fifo_cache.Get(1), 2);
}

/// @brief Test removal of existing keys.
/// @note Verifies successful removal of existing entries and subsequent cache state.
TEST(FIFOCacheTest, RemoveExistingKey) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  FIFOCache<int, int> fifo_cache(std::move(basic_cache));

  fifo_cache.Put(1, 1);
  ASSERT_TRUE(fifo_cache.Remove(1));
  ASSERT_EQ(fifo_cache.Get(1), std::nullopt);
  ASSERT_EQ(fifo_cache.Size(), 0);
}

/// @brief Test removal of non-existent keys.
/// @note Verifies cache behavior when attempting to remove non-existing keys.
TEST(FIFOCacheTest, RemoveNonExistingKey) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  FIFOCache<int, int> fifo_cache(std::move(basic_cache));

  ASSERT_FALSE(fifo_cache.Remove(1));
  ASSERT_EQ(fifo_cache.Size(), 0);
}

/// @brief Test complete cache clearance.
/// @note Verifies that Clear() operation removes all entries and resets cache state.
TEST(FIFOCacheTest, ClearCache) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  FIFOCache<int, int> fifo_cache(std::move(basic_cache));

  fifo_cache.Put(1, 1);
  fifo_cache.Put(2, 2);
  fifo_cache.Clear();

  ASSERT_EQ(fifo_cache.Size(), 0);
  ASSERT_EQ(fifo_cache.Get(1), std::nullopt);
  ASSERT_EQ(fifo_cache.Get(2), std::nullopt);
}

/// @brief Test FIFO eviction policy when capacity is exceeded.
/// @note Verifies that oldest entries are evicted according to FIFO policy when cache reaches capacity.
TEST(FIFOCacheTest, EvictOldestKeyWhenFull) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  FIFOCache<int, int> fifo_cache(std::move(basic_cache), 3, 1);

  // Insert 3 entries
  fifo_cache.Put(1, 1);
  fifo_cache.Put(2, 2);
  fifo_cache.Put(3, 3);

  // Access key 1 - should not change FIFO order
  ASSERT_EQ(fifo_cache.Get(1), 1);

  // Insert 4th item - should evict oldest (key 1)
  fifo_cache.Put(4, 4);

  // Verify eviction
  ASSERT_EQ(fifo_cache.Get(1), std::nullopt);  // Oldest key (1) should be evicted
  ASSERT_EQ(fifo_cache.Get(2), 2);             // Should remain
  ASSERT_EQ(fifo_cache.Get(3), 3);             // Should remain
  ASSERT_EQ(fifo_cache.Get(4), 4);             // Newly added should remain
  ASSERT_EQ(fifo_cache.Size(), 3);
}

/// @brief Test concurrent Put operations under high contention.
/// @note Verifies that concurrent operations maintain data consistency.
TEST(FIFOCacheTest, ConcurrentPut) {
  constexpr int capacity = 1000;
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  FIFOCache<int, int> fifo_cache(std::move(basic_cache), capacity, 32);

  constexpr int threads_num = 8;
  constexpr int puts_per_thread = 200;
  constexpr int puts_num = threads_num * puts_per_thread;

  std::vector<std::thread> threads;
  std::atomic<int> key_gen{0};

  for (int i = 0; i < threads_num; ++i) {
    threads.emplace_back([&]() {
      for (int j = 0; j < puts_per_thread; ++j) {
        int key = key_gen.fetch_add(1, std::memory_order_relaxed);
        fifo_cache.Put(key, key);
      }
    });
  }
  for (auto& t : threads) t.join();

  int hits{0}, misses{0};

  for (int key = 0; key < puts_num; ++key) {
    if (fifo_cache.Get(key) == std::nullopt) {
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
TEST(FIFOCacheTest, ConcurrentGet) {
  const int capacity = 1000;
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  FIFOCache<int, int> fifo_cache(std::move(basic_cache), capacity);

  for (int key = 0; key < capacity; ++key) {
    fifo_cache.Put(key, key);
  }

  constexpr int threads_num = 8;
  constexpr int gets_per_thread = 200;
  std::vector<std::thread> threads;

  for (int i = 0; i < threads_num; ++i) {
    threads.emplace_back([&, i]() {
      for (int j = 0; j < gets_per_thread; ++j) {
        int key = j + i * gets_per_thread;
        if (key < capacity) {
          ASSERT_EQ(fifo_cache.Get(key), key);
        } else {
          ASSERT_EQ(fifo_cache.Get(key), std::nullopt);
        }
      }
    });
  }
  for (auto& t : threads) t.join();
}

}  // namespace trpc::cache::testing
