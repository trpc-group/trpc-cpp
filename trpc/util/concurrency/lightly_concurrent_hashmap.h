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

#include "trpc/util/concurrency/detail/lightly_concurrent_hashmap_impl.h"

namespace trpc::concurrency {

template <
    typename KeyType,
    typename ValueType,
    typename HashFn = std::hash<KeyType>,
    typename KeyEqual = std::equal_to<KeyType>>
using LightlyConcurrentHashMap = detail::LightlyConcurrentHashMapImpl<
    KeyType,
    ValueType,
    HashFn,
    KeyEqual>;

}  // namespace trpc::concurrency
