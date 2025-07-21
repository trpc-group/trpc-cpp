//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/function_view_test.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/function_view.h"

#include <array>
#include <functional>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/util/function.h"

namespace trpc::testing {

int PlainOldFunction(int, double, char) { return 1; }

int CallThroughFunctionView(FunctionView<int(int, double, char)> fv, int x, double y, char z) { return fv(x, y, z); }

int CallThroughFunctionView2(FunctionView<int()> fv) { return fv(); }

TEST(FunctionViewTest, POF) { ASSERT_EQ(1, (CallThroughFunctionView(PlainOldFunction, 0, 0, 0))); }

TEST(FunctionViewTest, POFPtr) {
  Function<int(int, double, char)> fv(PlainOldFunction);
  FunctionView<int(int, double, char)> f1(fv);
  ASSERT_TRUE(f1);
  FunctionView<int(int, double, char)> f2;
  ASSERT_FALSE(f2);
  ASSERT_EQ(1, (CallThroughFunctionView(&PlainOldFunction, 0, 0, 0)));
  ASSERT_EQ(1, (f1(0, 0, 0)));
  ASSERT_EQ(1, (fv(0, 0, 0)));
}

TEST(FunctionViewTest, Lambda) {
  ASSERT_EQ(1, CallThroughFunctionView2([] { return 1; }));
}

struct ConstOperatorCall {
  int operator()() const { return 1; }
};
struct NonconstOperatorCall {
  int operator()() { return 1; }
};
struct NonconstNoexceptOperatorCall {
  int operator()() noexcept { return 1; }
};

TEST(FunctionViewTest, ConstnessCorrect) {
  ASSERT_EQ(1, CallThroughFunctionView2(ConstOperatorCall()));
  ASSERT_EQ(1, CallThroughFunctionView2(NonconstOperatorCall()));
  ASSERT_EQ(1, CallThroughFunctionView2(NonconstNoexceptOperatorCall()));
}

class FancyClass {
 public:
  int f(int x) { return x; }
};

int CallThroughFunctionView3(FunctionView<int(FancyClass*, int)> fv, FancyClass* fc, int x) { return fv(fc, x); }

TEST(FunctionViewTest, MemberMethod) {
  FancyClass fc;
  ASSERT_EQ(10, CallThroughFunctionView3(&FancyClass::f, &fc, 10));
}

void CallThroughFunctionView4(FunctionView<int()> f) { f(); }

TEST(FunctionViewTest, CastAnyTypeToVoid) {
  int x = 0;
  CallThroughFunctionView4([&x]() -> int {
    x = 1;
    return x;
  });
  ASSERT_EQ(1, x);
}

}  // namespace trpc::testing
