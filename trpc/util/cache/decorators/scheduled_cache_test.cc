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

#include "trpc/util/cache/decorators/scheduled_cache.h"
#include "trpc/util/cache/impl/basic_cache.h"

#include "gtest/gtest.h"

namespace trpc::cache::testing {

/// @brief Test basic Put/Get operations and size tracking.
/// @note Verifies fundamental cache operations including insertion, retrieval,
///       and size maintenance functionality.
TEST(ScheduledCacheTest, BasicPutGetWithTTL) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  ScheduledCache<int, int> scheduled_cache(std::move(basic_cache), std::chrono::milliseconds(100));

  ASSERT_EQ(scheduled_cache.Size(), 0);
  ASSERT_TRUE(scheduled_cache.Put(1, 1));
  ASSERT_EQ(scheduled_cache.Get(1), 1);
  ASSERT_EQ(scheduled_cache.Size(), 1);
}

/// @brief Test handling of duplicate key insertion.
/// @note Verifies that inserting existing keys updates values correctly
///       without affecting cache size.
TEST(ScheduledCacheTest, PutDuplicateKey) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  ScheduledCache<int, int> scheduled_cache(std::move(basic_cache), std::chrono::milliseconds(100));

  ASSERT_TRUE(scheduled_cache.Put(1, 1));
  ASSERT_FALSE(scheduled_cache.Put(1, 2));
  ASSERT_EQ(scheduled_cache.Size(), 1);
  ASSERT_EQ(scheduled_cache.Get(1), 2);
}

/// @brief Test removal of existing keys.
/// @note Verifies successful removal of existing entries and subsequent cache state.
TEST(ScheduledCacheTest, RemoveExistingKey) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  ScheduledCache<int, int> scheduled_cache(std::move(basic_cache));

  scheduled_cache.Put(1, 1);
  ASSERT_TRUE(scheduled_cache.Remove(1));
  ASSERT_EQ(scheduled_cache.Get(1), std::nullopt);
  ASSERT_EQ(scheduled_cache.Size(), 0);
}

/// @brief Test removal of non-existent keys.
/// @note Verifies cache behavior when attempting to remove non-existing keys.
TEST(ScheduledCacheTest, RemoveNonExistingKey) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  ScheduledCache<int, int> scheduled_cache(std::move(basic_cache));

  ASSERT_FALSE(scheduled_cache.Remove(1));
  ASSERT_EQ(scheduled_cache.Size(), 0);
}

/// @brief Test complete cache clearance.
/// @note Verifies that Clear() operation removes all entries and resets cache state.
TEST(ScheduledCacheTest, ClearCache) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  ScheduledCache<int, int> scheduled_cache(std::move(basic_cache));

  scheduled_cache.Put(1, 1);
  scheduled_cache.Put(2, 2);
  scheduled_cache.Clear();

  ASSERT_EQ(scheduled_cache.Size(), 0);
  ASSERT_EQ(scheduled_cache.Get(1), std::nullopt);
  ASSERT_EQ(scheduled_cache.Get(2), std::nullopt);
}

/// @brief Test automatic expiration of entries based on TTL.
/// @note Verifies that entries are automatically evicted after their TTL expires,
///       and cache size is updated accordingly.
TEST(ScheduledCacheTest, EntryExpiration) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  ScheduledCache<int, int> scheduled_cache(std::move(basic_cache), std::chrono::milliseconds(50));

  ASSERT_TRUE(scheduled_cache.Put(1, 1));
  ASSERT_EQ(scheduled_cache.Get(1), 1);

  // Wait for entry to expire
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  ASSERT_EQ(scheduled_cache.Get(1), std::nullopt);
  ASSERT_EQ(scheduled_cache.Size(), 0);
}

/// @brief Test custom TTL per entry.
/// @note Verifies that individual entries can have custom TTL values,
//       and expiration is handled correctly for each.
TEST(ScheduledCacheTest, CustomTTLPerEntry) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  ScheduledCache<int, int> scheduled_cache(std::move(basic_cache), std::chrono::milliseconds(1000));

  // Short TTL
  ASSERT_TRUE(scheduled_cache.Put(1, 1, std::chrono::milliseconds(50)));
  // Long TTL
  ASSERT_TRUE(scheduled_cache.Put(2, 2, std::chrono::milliseconds(200)));

  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  ASSERT_EQ(scheduled_cache.Get(1), std::nullopt);  // Should be expired
  ASSERT_EQ(scheduled_cache.Get(2), 2);             // Should still be valid

  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  ASSERT_EQ(scheduled_cache.Get(2), std::nullopt);  // Should be expired
}

/// @brief Test concurrent Put operations with TTL.
/// @note Verifies that concurrent Put operations maintain data consistency
///       and TTL expiration under high contention.

TEST(ScheduledCacheTest, ConcurrentPut) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  ScheduledCache<int, int> scheduled_cache(std::move(basic_cache), std::chrono::milliseconds(1000), 32);

  constexpr int threads_num = 8;
  constexpr int puts_per_thread = 200;
  constexpr int puts_num = threads_num * puts_per_thread;
  std::vector<std::thread> threads;
  std::atomic<int> key_gen{0};

  for (int i = 0; i < threads_num; ++i) {
    threads.emplace_back([&]() {
      for (int j = 0; j < puts_per_thread; ++j) {
        int key = key_gen.fetch_add(1, std::memory_order_relaxed);
        scheduled_cache.Put(key, key);
      }
    });
  }
  for (auto& t : threads) t.join();

  int hits{0}, misses{0};
  for (int key = 0; key < puts_num; ++key) {
    if (scheduled_cache.Get(key) == std::nullopt) {
      ++misses;
    } else {
      ++hits;
    }
  }

  ASSERT_EQ(hits, threads_num * puts_per_thread);
  ASSERT_EQ(misses, 0);
}

/// @brief Test concurrent Get operations with expiration.
/// @note Verifies that concurrent Get operations handle expiration correctly,
///       with some gets succeeding before expiration and some failing after.
TEST(ScheduledCacheTest, ConcurrentGetWithExpiration) {
  auto basic_cache = std::make_unique<BasicCache<int, int>>();
  ScheduledCache<int, int> scheduled_cache(std::move(basic_cache), std::chrono::milliseconds(50), 32);

  for (int key = 0; key < 100; ++key) {
    scheduled_cache.Put(key, key);
  }

  constexpr int threads_num = 4;
  constexpr int gets_per_thread = 25;
  std::vector<std::thread> threads;
  std::atomic<int> valid_gets{0};

  for (int i = 0; i < threads_num; ++i) {
    threads.emplace_back([&, i]() {
      for (int j = 0; j < gets_per_thread; ++j) {
        int key = j + i * gets_per_thread;
        if (scheduled_cache.Get(key)) {
          valid_gets.fetch_add(1, std::memory_order_relaxed);
        }
      }
    });
  }
  for (auto& t : threads) t.join();

  // Some gets should succeed (before expiration), some should fail (after expiration)
  ASSERT_GT(valid_gets.load(), 0);
  ASSERT_LE(valid_gets.load(), 100);
}

}  // namespace trpc::cache::testing
