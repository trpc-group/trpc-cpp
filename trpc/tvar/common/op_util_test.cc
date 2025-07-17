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

#include "trpc/tvar/common/op_util.h"

#include <cstdint>
#include <type_traits>

#include "gtest/gtest.h"

namespace {

using trpc::tvar::add_cr_non_integral_t;
using trpc::tvar::is_void_op;
using trpc::tvar::VoidOp;

}  // namespace

namespace trpc::testing {

/// @brief Test for add_cr_non_integral_t.
TEST(TypeTraitsTest, TestAddCrNonIntegral) {
  ASSERT_TRUE((std::is_same_v<bool, add_cr_non_integral_t<bool>>));
  ASSERT_TRUE((std::is_same_v<char, add_cr_non_integral_t<char>>));
  ASSERT_TRUE((std::is_same_v<char16_t, add_cr_non_integral_t<char16_t>>));
  ASSERT_TRUE((std::is_same_v<char32_t, add_cr_non_integral_t<char32_t>>));
  ASSERT_TRUE((std::is_same_v<wchar_t, add_cr_non_integral_t<wchar_t>>));
  ASSERT_TRUE((std::is_same_v<int, add_cr_non_integral_t<int>>));
  ASSERT_TRUE((std::is_same_v<int8_t, add_cr_non_integral_t<int8_t>>));
  ASSERT_TRUE((std::is_same_v<int16_t, add_cr_non_integral_t<int16_t>>));
  ASSERT_TRUE((std::is_same_v<int32_t, add_cr_non_integral_t<int32_t>>));
  ASSERT_TRUE((std::is_same_v<int64_t, add_cr_non_integral_t<int64_t>>));
  ASSERT_TRUE((std::is_same_v<uint8_t, add_cr_non_integral_t<uint8_t>>));
  ASSERT_TRUE((std::is_same_v<uint16_t, add_cr_non_integral_t<uint16_t>>));
  ASSERT_TRUE((std::is_same_v<uint32_t, add_cr_non_integral_t<uint32_t>>));
  ASSERT_TRUE((std::is_same_v<uint64_t, add_cr_non_integral_t<uint64_t>>));

  ASSERT_FALSE((std::is_same_v<float, add_cr_non_integral_t<float>>));
  ASSERT_FALSE((std::is_same_v<double, add_cr_non_integral_t<double>>));
}

/// @brief Test for is_void_op.
TEST(TypeTraitsTest, TestIsVoidOp) {
  ASSERT_TRUE(is_void_op<VoidOp>::value);
  ASSERT_FALSE(is_void_op<int>::value);
}

}  // namespace trpc::testing
