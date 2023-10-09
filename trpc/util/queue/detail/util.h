//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include <cstdint>
#include <memory>

namespace trpc::queue::detail {

inline void Pause() {
#if defined(__x86_64__)
  asm volatile("pause" ::: "memory");
#elif defined(__aarch64__)
  asm volatile("yield" ::: "memory");
#elif defined(__powerpc__)
  asm volatile("or 31,31,31   # very low priority" ::: "memory");
#else
#error Unsupported architecture.
#endif
}

}  // namespace trpc::queue::detail
