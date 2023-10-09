//
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/hazptr/entry_cache.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/hazptr/entry_cache.h"

#include <array>

#include "trpc/util/log/logging.h"

namespace trpc::hazptr {

inline constexpr std::size_t kEntryCacheSize = 8;

struct EntryCacheSlow {
  std::array<Entry*, kEntryCacheSize> cache;
  Entry** current = cache.data();
  Entry** top = cache.data() + cache.size();

  ~EntryCacheSlow() {
    while (current != cache.data()) {
      PutEntryOfDefaultDomain(*--current);
    }
  }
};

EntryCacheSlow* GetEntryCacheSlow() {
  thread_local EntryCacheSlow cache;
  return &cache;
}

void EntryCache::InitializeOnceCheck() {
  if (current == nullptr) {
    auto&& cache = GetEntryCacheSlow();
    TRPC_CHECK(top == nullptr);
    TRPC_CHECK(bottom == nullptr);
    current = cache->current;
    bottom = cache->cache.data();
    top = cache->cache.data() + cache->cache.size();
  }
}

Entry* EntryCache::GetSlow() noexcept {
  InitializeOnceCheck();
  return GetEntryOfDefaultDomain();
}

void EntryCache::PutSlow(Entry* entry) noexcept {
  InitializeOnceCheck();
  return PutEntryOfDefaultDomain(entry);
}

Entry* GetEntryOfDefaultDomain() noexcept { return GetDefaultHazptrDomain()->GetEntry(); }

void PutEntryOfDefaultDomain(Entry* entry) noexcept { GetDefaultHazptrDomain()->PutEntry(entry); }

}  // namespace trpc::hazptr
