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

#include "trpc/util/cache/impl/basic_cache.h"
#include "gtest/gtest.h"

namespace trpc::cache::testing {

/// @brief Test basic Put/Get operations and size tracking.
/// @note Verifies fundamental cache operations including insertion, retrieval,
///       and size maintenance functionality.
TEST(BasicCacheTest, BasicPutGetSize) {
  BasicCache<int, int> basic_cache;

  ASSERT_EQ(basic_cache.Size(), 0);
  ASSERT_TRUE(basic_cache.Put(1, 1));
  ASSERT_EQ(basic_cache.Get(1), 1);
  ASSERT_EQ(basic_cache.Size(), 1);

  // Test updating existing key
  ASSERT_FALSE(basic_cache.Put(1, 2));
  ASSERT_EQ(basic_cache.Get(1), 2);
  ASSERT_EQ(basic_cache.Size(), 1);  // Size should not change on update
}

/// @brief Test handling of duplicate key insertion.
/// @note Verifies that inserting existing keys updates values correctly
///       without affecting cache size.
TEST(BasicCacheTest, PutDuplicateKey) {
  BasicCache<int, int> basic_cache;

  ASSERT_TRUE(basic_cache.Put(1, 1));
  ASSERT_FALSE(basic_cache.Put(1, 2));
  ASSERT_EQ(basic_cache.Get(1), 2);
  ASSERT_EQ(basic_cache.Size(), 1);
}

/// @brief Test removal of existing keys.
/// @note Verifies successful removal of existing items and subsequent cache state.
TEST(BasicCacheTest, RemoveExistingKey) {
  BasicCache<int, int> basic_cache;

  basic_cache.Put(1, 1);
  ASSERT_TRUE(basic_cache.Remove(1));
  ASSERT_EQ(basic_cache.Get(1), std::nullopt);
  ASSERT_EQ(basic_cache.Size(), 0);
}

/// @brief Test removal of non-existent keys.
/// @note Verifies cache behavior when attempting to remove non-existing keys.
TEST(BasicCacheTest, RemoveNonExistingKey) {
  BasicCache<int, int> basic_cache;

  ASSERT_FALSE(basic_cache.Remove(1));
  ASSERT_EQ(basic_cache.Size(), 0);
}

/// @brief Test complete cache clearance.
/// @note Verifies that Clear() operation removes all items and resets cache state.
TEST(BasicCacheTest, ClearCache) {
  BasicCache<int, int> basic_cache;

  basic_cache.Put(1, 1);
  basic_cache.Put(2, 2);
  basic_cache.Clear();

  ASSERT_EQ(basic_cache.Size(), 0);
  ASSERT_EQ(basic_cache.Get(1), std::nullopt);
  ASSERT_EQ(basic_cache.Get(2), std::nullopt);
}

/// @brief Test concurrent Put operations under high contention.
/// @note Verifies that concurrent operations maintain data consistency.
TEST(BasicCacheTest, ConcurrentPut) {
  BasicCache<int, int> basic_cache;
  constexpr int threads_num = 8;
  constexpr int puts_per_thread = 200;
  constexpr int puts_num = threads_num * puts_per_thread;
  std::vector<std::thread> threads;
  std::atomic<int> key_gen{0};

  for (int i = 0; i < threads_num; ++i) {
    threads.emplace_back([&]() {
      for (int j = 0; j < puts_per_thread; ++j) {
        int key = key_gen.fetch_add(1, std::memory_order_relaxed);
        basic_cache.Put(key, key);
      }
    });
  }
  for (auto& t : threads) t.join();

  int hits{0}, misses{0};

  for (int key = 0; key < puts_num; ++key) {
    if (basic_cache.Get(key) == std::nullopt) {
      ++misses;
    } else {
      ++hits;
    }
  }

  ASSERT_EQ(hits, puts_num);
  ASSERT_EQ(misses, 0);
}

}  // namespace trpc::cache::testing
