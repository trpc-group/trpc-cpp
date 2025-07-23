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

#include <cstddef>
#include <cstdint>

/* Prevent the compiler from merging or refetching accesses.
 * The compiler is also forbidden from reordering successive instances of
 * ACCESS_ONCE(), but only when the compiler is aware of some particular ordering.
 * One way to make the compiler aware of ordering is to put the two invocations
 * of ACCESS_ONCE() in different C statements.
 *
 * This macro does absolutely -nothing- to prevent the CPU from reordering,
 * merging, or refetching absolutely anything at any time.  Its main intended
 * use is to mediate communication between process-level code and irq/NMI
 * handlers, all running on the same CPU.
 */
#define TRPC_ACCESS_ONCE(x) (*(volatile decltype(x)*)&(x))

#if defined(__x86_64__)
#define TRPC_CPU_RELAX() asm volatile("pause" ::: "memory")
#elif defined(__aarch64__)
#define TRPC_CPU_RELAX() asm volatile("yield\n" : : : "memory")
#else
#define TRPC_CPU_RELAX()
#endif
