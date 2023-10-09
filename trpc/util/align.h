//
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/align.h.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <cstddef>

namespace trpc {

constexpr std::size_t max_align_v = alignof(max_align_t);

// At the time of writing, GCC 8.2 has not implemented these constants.
#if defined(__x86_64__)

// On Sandy Bridge, accessing adjacent cache lines also sees destructive
// interference.
//
// @sa: https://github.com/facebook/folly/blob/master/folly/lang/Align.h
constexpr std::size_t hardware_destructive_interference_size = 128;
constexpr std::size_t hardware_constructive_interference_size = 64;

#elif defined(__aarch64__)

// AArch64 is ... weird, to say the least. Some vender (notably Samsung) uses a
// non-consistent cacheline size across BIG / little cores..
//
// Let's ignore those CPUs for now.
//
// @sa: https://www.mono-project.com/news/2016/09/12/arm64-icache/
constexpr std::size_t hardware_destructive_interference_size = 64;
constexpr std::size_t hardware_constructive_interference_size = 64;

#elif defined(__powerpc64__)

// These values are read from
// `/sys/devices/system/cpu/cpu0/cache/index*/coherency_line_size`
constexpr std::size_t hardware_destructive_interference_size = 128;
constexpr std::size_t hardware_constructive_interference_size = 128;

#else

#error Unsupported architecture.

#endif

}  // namespace trpc
