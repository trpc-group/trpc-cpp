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

#include "trpc/util/singleton.h"

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(Singleton, Dead1) {
  class A : public Singleton<A> {};

  class B : public Singleton<B> {
   public:
    B() { EXPECT_NE(A::GetInstance(), nullptr); }
    ~B() { EXPECT_NE(A::GetInstance(), nullptr); }
  };

  ASSERT_NE(B::GetInstance(), nullptr);
  ASSERT_NE(A::GetInstance(), nullptr);
}

TEST(Singleton, Dead2) {
  class A : public Singleton<A, CreateUsingNew, NoDestroyLifetime> {};

  class B : public Singleton<B> {
   public:
    B() {}
    ~B() { EXPECT_NE(A::GetInstance(), nullptr); }
  };

  ASSERT_NE(B::GetInstance(), nullptr);
  ASSERT_NE(A::GetInstance(), nullptr);
}

}  // namespace trpc::testing
