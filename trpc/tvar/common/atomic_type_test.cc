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

#include "trpc/tvar/common/atomic_type.h"

#include <cstdint>

#include "gtest/gtest.h"

namespace {

using trpc::tvar::AtomicType;

class MyClass {
 public:
  MyClass() = default;

  MyClass(int64_t l, double r) : a(l), b{r} {}

  int64_t a{0};

  double b{0.0};
};

struct MyAddOp {
  void operator()(int64_t* left, int64_t right) const { *left += right; }
};

struct MyClassOp {
  void operator()(MyClass* left, MyClass const& right) const {
    left->a += right.a;
    left->b += right.b;
  }
};

}  // namespace

namespace trpc::testing {

/// @brief Test sizeof template.
TEST(AtomicTypeTest, TestTemplate) {
  ASSERT_EQ(sizeof(int32_t), sizeof(AtomicType<int32_t>));
  ASSERT_EQ(sizeof(int64_t), sizeof(AtomicType<int64_t>));
  ASSERT_EQ(sizeof(float), sizeof(AtomicType<float>));
  ASSERT_EQ(sizeof(double), sizeof(AtomicType<double>));

  ASSERT_LT(sizeof(int*), sizeof(AtomicType<int*>));
  ASSERT_LT(sizeof(MyClass), sizeof(AtomicType<MyClass>));
}

/// @brief Test standard atomic implementation.
TEST(AtomicTypeTest, TestAtomic) {
  AtomicType<int64_t> value;

  value.Store(0);
  ASSERT_EQ(0, value.Load());

  {
    int64_t ex{-1};
    value.Exchange(&ex, 10);
    ASSERT_EQ(0, ex);
    ASSERT_EQ(10, value.Load());
  }

  int64_t ex{3};
  value.CompareExchangeWeak(&ex, 8);
  ASSERT_EQ(10, ex);
  value.CompareExchangeWeak(&ex, 8);
  ASSERT_EQ(8, value.Load());

  value.Modify<MyAddOp>(3);
  ASSERT_EQ(11, value.Load());
}

/// @brief Test mutex implementation.
TEST(AtomicTypeTest, TestMutex) {
  AtomicType<MyClass> value;

  {
    value.Store(MyClass(3, 3.0));
    auto v = value.Load();
    ASSERT_EQ(3, v.a);
    ASSERT_EQ(3.0, v.b);
  }

  {
    MyClass ex;
    value.Exchange(&ex, MyClass(10, 10.0));
    ASSERT_EQ(3, ex.a);
    ASSERT_EQ(3.0, ex.b);
    auto v = value.Load();
    ASSERT_EQ(10, v.a);
    ASSERT_EQ(10.0, v.b);
  }

  value.Modify<MyClassOp>(MyClass(3, 3.0));
  auto v = value.Load();
  ASSERT_EQ(13, v.a);
  ASSERT_EQ(13.0, v.b);
}

}  // namespace trpc::testing
