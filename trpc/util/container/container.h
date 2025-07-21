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

#pragma once

#include <list>
#include <map>
#include <memory>
#include <unordered_map>

#include "trpc/util/container/fixed_arena_allocator.h"

namespace trpc::container {

#ifndef __clang__

/// @brief std::list with pre-allocated size.
template <typename T, std::size_t kCacheItemSize>
using FixedList = std::list<T, FixedArenaAllocator<T, kCacheItemSize>>;

/// @brief std::unordered_map with pre-allocated size.
template <typename Key, typename T, std::size_t kCacheItemSize>
using FixedUnorderedMap = std::unordered_map<Key,
                                             T,
                                             std::hash<Key>,
                                             std::equal_to<Key>,
                                             FixedArenaAllocator<std::pair<const Key, T>, kCacheItemSize>>;

/// @brief std::multimap with pre-allocated size.
template <typename Key, typename T, std::size_t kCacheItemSize>
using FixedMultiMap = std::multimap<Key,
                                    T,
                                    std::less<Key>,
                                    FixedArenaAllocator<std::pair<const Key, T>, kCacheItemSize>>;

#else

/// @brief Use libstdc++ if compiled by clang.
template <typename T, std::size_t kCacheItemSize>
using FixedList = std::list<T>;

/// @brief Use libstdc++ if compiled by clang.
template <typename Key, typename T, std::size_t kCacheItemSize>
using FixedUnorderedMap = std::unordered_map<Key, T, std::hash<Key>, std::equal_to<Key>>;

/// @brief Use libstdc++ if compiled by clang.
template <typename Key, typename T, std::size_t kCacheItemSize>
using FixedMultiMap = std::multimap<Key, T, std::less<Key>>;

#endif

}  // namespace trpc::container
