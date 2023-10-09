//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/demangle_test.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/internal/demangle.h"

#include "gtest/gtest.h"

namespace trpc::internal::testing {

struct C {
  struct D {
    struct E {};
  };
};

TEST(Demangle, All) {
  ASSERT_EQ("trpc::internal::testing::C::D::E", GetTypeName<C::D::E>());
  ASSERT_NE(GetTypeName<C::D::E>(), typeid(C::D::E).name());
  ASSERT_EQ("invalid function name !@#$", Demangle("invalid function name !@#$"));
}

}  // namespace trpc::internal::testing
