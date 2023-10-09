//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/delayed_init_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/delayed_init.h"

#include "gtest/gtest.h"

using namespace std::literals;

namespace trpc::testing {

bool initialized = false;

struct DefaultConstructibleInitialization {
  DefaultConstructibleInitialization() { initialized = true; }
};

struct InitializeCtorArgument {
  explicit InitializeCtorArgument(bool* ptr) { *ptr = true; }
};

TEST(DelayedInit, DefaultConstructible) {
  DelayedInit<DefaultConstructibleInitialization> dc;
  ASSERT_FALSE(initialized);
  dc.Init();
  ASSERT_TRUE(initialized);
}

TEST(DelayedInit, InitializeCtorArgument) {
  bool f = false;
  DelayedInit<InitializeCtorArgument> ica;
  ASSERT_FALSE(f);
  ica.Init(&f);
  ASSERT_TRUE(f);
}

}  // namespace trpc::testing
