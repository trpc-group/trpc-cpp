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

#include <algorithm>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/util/thread/process_info.h"

namespace trpc::testing {

TEST(SystemInfoTest, GetProcessCpuQuota) {
  // Normal testing.
  ASSERT_LT(1e-6, GetProcessCpuQuota());

  // Test getting the time in a non-container environment.
  ASSERT_LT(1e-6, GetProcessCpuQuota([]() { return false; }));

  // Test getting the time in a container environment.
  ASSERT_LT(1e-6, GetProcessCpuQuota([]() { return true; }));
}

TEST(SystemInfoTest, GetProcessMemoryQuota) {
  // Normal testing.
  ASSERT_NE(0, GetProcessMemoryQuota());

  // Test getting the time in a non-container environment.
  ASSERT_NE(0, GetProcessMemoryQuota([]() { return false; }));

  // Test getting the time in a container environment.
  ASSERT_NE(0, GetProcessMemoryQuota([]() { return true; }));
}
}  // namespace trpc::testing
