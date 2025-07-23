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

#include "trpc/util/object_pool/disabled.h"

#include <stdlib.h>

#include <string>

#include "gtest/gtest.h"

#include "trpc/util/object_pool/object_pool.h"

namespace trpc::object_pool {

int alive = 0;

struct C {
  C() { ++alive; }
  ~C() { --alive; }
  int a;
  std::string s;
};

template <>
struct ObjectPoolTraits<C> {
  static constexpr auto kType = ObjectPoolType::kDisabled;
};

TEST(DisabledPool, All) {
  C* ptr = trpc::object_pool::New<C>();
  ptr->a = 1;
  ptr->s = "hello";

  ASSERT_EQ(1, alive);

  trpc::object_pool::Delete<C>(ptr);

  ASSERT_EQ(0, alive);
}

}  // namespace trpc::object_pool
