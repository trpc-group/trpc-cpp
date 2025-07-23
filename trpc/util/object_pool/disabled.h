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

#include <cstdlib>

namespace trpc::object_pool::disabled {

template <typename T>
T* New() {
  return static_cast<T*>(aligned_alloc(alignof(T), sizeof(T)));
}

template <typename T>
void Delete(T* ptr) {
  free(static_cast<void*>(ptr));
}

}  // namespace trpc::object_pool::disabled
