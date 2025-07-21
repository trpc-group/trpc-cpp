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

#include "trpc/util/concurrency/detail/lightly_concurrent_hashmap_impl.h"

#include <random>
#include <thread>

#include "gtest/gtest.h"

namespace trpc::concurrency::detail::testing {

std::atomic<int> g_insert_num = 0;
std::atomic<int> g_erase_num = 0;
std::atomic<bool> g_start = false;

void DoInsert(int index, LightlyConcurrentHashMapImpl<int, int>* hash_table, int num) {
  while (!g_start) {
    std::this_thread::yield();
  }

  auto begin = std::chrono::steady_clock::now();

  for (int i = 0; i < num; ++i) {
    int x = rand() % num;
    if (hash_table->Insert(x, x)) {
      ++g_insert_num;
    }
  }
  auto end = std::chrono::steady_clock::now();

  int ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

  std::cout << "Insert thread index: " << index
            << ", timecost(ms): " << ms
            << std::endl;
}

void DoGet(int index, LightlyConcurrentHashMapImpl<int, int>* hash_table, int num) {
  while (!g_start) {
    std::this_thread::yield();
  }

  ::usleep(10000);

  auto begin = std::chrono::steady_clock::now();

  for (int i = 0; i < num; ++i) {
    int x = rand() % num;
    int value = 0;
    if (hash_table->Get(x, value)) {
      ASSERT_TRUE(x == value);
    }
  }
  auto end = std::chrono::steady_clock::now();

  int ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

  std::cout << "Get    thread index: " << index
            << ", timecost(ms): " << ms
            << std::endl;
}

void DoErase(int index, LightlyConcurrentHashMapImpl<int, int>* hash_table, int num) {
  while (!g_start) {
    std::this_thread::yield();
  }

  ::usleep(100000);

  auto begin = std::chrono::steady_clock::now();

  for (int i = 0; i < num; ++i) {
    int x = rand() % num;
    if (hash_table->Erase(x)) {
      ++g_erase_num;
    }
  }
  auto end = std::chrono::steady_clock::now();

  int ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

  std::cout << "Erase  thread index: " << index
            << ", timecost(ms): " << ms
            << std::endl;
}

void DoGetOrInsert(int index, LightlyConcurrentHashMapImpl<int, int>* hash_table, int num) {
  while (!g_start) {
    std::this_thread::yield();
  }

  auto begin = std::chrono::steady_clock::now();

  for (int i = 0; i < num; ++i) {
    int x = rand() % num;
    int value = x;
    int exist_value = 0;
    if (hash_table->GetOrInsert(x, value, exist_value)) {
      ASSERT_TRUE(x == exist_value);
    } else {
      ++g_insert_num;
    }
  }
  auto end = std::chrono::steady_clock::now();

  int ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

  std::cout << "GetOrInsert thread index: " << index
            << ", timecost(ms): " << ms
            << std::endl;
}

void DoInsertOrAssign(int index, LightlyConcurrentHashMapImpl<int, int>* hash_table, int num) {
  while (!g_start) {
    std::this_thread::yield();
  }

  auto begin = std::chrono::steady_clock::now();

  for (int i = 0; i < num; ++i) {
    int x = rand() % num;
    if (hash_table->InsertOrAssign(x, x)) {
      ++g_insert_num;
    }
  }
  auto end = std::chrono::steady_clock::now();

  int ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

  std::cout << "GetOrInsert thread index: " << index
            << ", timecost(ms): " << ms
            << std::endl;
}


TEST(LightlyConcurrentHashMapImplTest, ALL) {
  g_insert_num = 0;
  g_erase_num = 0;
  g_start = false;

  LightlyConcurrentHashMapImpl<int, int> hash_table(1024);

  std::vector<std::thread> insert_threads;
  for (int i = 0; i < 4; ++i) {
    insert_threads.push_back(std::thread(DoInsert, i, &hash_table, 100000));
  }

  std::vector<std::thread> get_threads;
  for (int i = 0; i < 8; ++i) {
    get_threads.push_back(std::thread(DoGet, i, &hash_table, 100000));
  }

  std::vector<std::thread> erase_threads;
  for (int i = 0; i < 4; ++i) {
    erase_threads.push_back(std::thread(DoErase, i, &hash_table, 100000));
  }

  g_start = true;

  for (int i = 0; i < 4; ++i) {
    insert_threads[i].join();
  }

  for (int i = 0; i < 8; ++i) {
    get_threads[i].join();
  }

  for (int i = 0; i < 4; ++i) {
    erase_threads[i].join();
  }

  std::cout << "insert num: " << g_insert_num << ", erase num: " << g_erase_num << std::endl;
  std::cout << "hashmap num: " << hash_table.Size() << std::endl;

  ASSERT_EQ((g_insert_num - g_erase_num), hash_table.Size());

  hash_table.Clear();
}

TEST(LightlyConcurrentHashMapImplTest, GetOrInsert) {
  g_insert_num = 0;
  g_erase_num = 0;
  g_start = false;

  LightlyConcurrentHashMapImpl<int, int> hash_table(1024);

  std::vector<std::thread> insert_threads;
  for (int i = 0; i < 4; ++i) {
    insert_threads.push_back(std::thread(DoInsert, i, &hash_table, 100000));
  }

  std::vector<std::thread> get_or_insert_threads;
  for (int i = 0; i < 4; ++i) {
    get_or_insert_threads.push_back(std::thread(DoGetOrInsert, i, &hash_table, 100000));
  }

  std::vector<std::thread> erase_threads;
  for (int i = 0; i < 4; ++i) {
    erase_threads.push_back(std::thread(DoErase, i, &hash_table, 100000));
  }

  g_start = true;

  for (int i = 0; i < 4; ++i) {
    insert_threads[i].join();
  }

  for (int i = 0; i < 4; ++i) {
    get_or_insert_threads[i].join();
  }

  for (int i = 0; i < 4; ++i) {
    erase_threads[i].join();
  }

  std::cout << "insert num: " << g_insert_num << ", erase num: " << g_erase_num << std::endl;
  std::cout << "hashmap num: " << hash_table.Size() << std::endl;

  ASSERT_EQ((g_insert_num - g_erase_num), hash_table.Size());

  hash_table.Clear();
}

TEST(LightlyConcurrentHashMapImplTest, InsertOrAssign) {
  g_insert_num = 0;
  g_erase_num = 0;
  g_start = false;

  LightlyConcurrentHashMapImpl<int, int> hash_table(1024);

  std::vector<std::thread> insert_threads;
  for (int i = 0; i < 4; ++i) {
    insert_threads.push_back(std::thread(DoInsert, i, &hash_table, 100000));
  }

  std::vector<std::thread> insert_or_assign_threads;
  for (int i = 0; i < 4; ++i) {
    insert_or_assign_threads.push_back(std::thread(DoInsertOrAssign, i, &hash_table, 100000));
  }

  std::vector<std::thread> erase_threads;
  for (int i = 0; i < 4; ++i) {
    erase_threads.push_back(std::thread(DoErase, i, &hash_table, 100000));
  }

  g_start = true;

  for (int i = 0; i < 4; ++i) {
    insert_threads[i].join();
  }

  for (int i = 0; i < 4; ++i) {
    insert_or_assign_threads[i].join();
  }

  for (int i = 0; i < 4; ++i) {
    erase_threads[i].join();
  }

  std::cout << "insert num: " << g_insert_num << ", erase num: " << g_erase_num << std::endl;
  std::cout << "hashmap num: " << hash_table.Size() << std::endl;

  ASSERT_EQ((g_insert_num - g_erase_num), hash_table.Size());

  hash_table.Clear();
}

}  // namespace trpc::concurrency::detail::testing
