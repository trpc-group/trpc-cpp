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

#include "trpc/tvar/common/series.h"

#include "gtest/gtest.h"

#include "trpc/tvar/common/op_util.h"

namespace trpc::testing {

/// @brief Test for operator add.
TEST(ProbablyAddition, True) {
  tvar::OpAdd<int> op_add;
  tvar::ProbablyAddition<int, tvar::OpAdd<int>> addition(op_add);
  ASSERT_TRUE(addition);
}

/// @brief Test for operator minus.
TEST(ProbablyAddition, False) {
  tvar::OpMin<int> op_min;
  tvar::ProbablyAddition<int, tvar::OpMin<int>> addition(op_min);
  ASSERT_FALSE(addition);
}

/// @brief Divide on integral with operator add.
TEST(DivideOnAddition, AddOpIntegral) {
  int data = 10;
  tvar::OpAdd<int> op_add;
  tvar::DivideOnAddition<int, tvar::OpAdd<int>>::InplaceDivide(data, op_add, 2);
  ASSERT_EQ(data, 5);
}

/// @brief Divide on float with operator add.
TEST(DivideOnAddition, AddOpFloating) {
  double data = 10.0;
  tvar::OpAdd<double> op_add;
  tvar::DivideOnAddition<double, tvar::OpAdd<double>>::InplaceDivide(data, op_add, 2);
  ASSERT_EQ(data, 5.0);
}

/// @brief Divide on float with operator max.
TEST(DivideOnAddition, NotAddOp) {
  double data = 10.0;
  tvar::OpMax<double> op_max;
  tvar::DivideOnAddition<double, tvar::OpMax<double>>::InplaceDivide(data, op_max, 2);
  ASSERT_EQ(data, 10.0);
}

/// @brief Store series data with operator add.
TEST(SeriesBase, AddOp) {
  tvar::SeriesBase<int, tvar::OpAdd<int>> series_base;
  int sec_per_day = 24 * 60 * 60;
  for (int i = 0; i < sec_per_day; ++i) {
    series_base.Append(i % 60);
  }
  auto series_map = series_base.GetSeries();
  ASSERT_EQ(series_map.size(), 5);
  ASSERT_EQ(series_map["now"][0], 59);
  ASSERT_EQ(series_map["latest_sec"][0], 59);
  ASSERT_EQ(series_map["latest_min"][0], 30);
  ASSERT_EQ(series_map["latest_hour"][0], 30);
  ASSERT_EQ(series_map["latest_day"][0], 30);
}

/// @brief Store series data with operator max.
TEST(SeriesBase, NotOp) {
  tvar::SeriesBase<int, tvar::OpMax<int>> series_base;
  int sec_per_day = 24 * 60 * 60;
  for (int i = 0; i < sec_per_day; ++i) {
    series_base.Append(i % 60);
  }
  auto series_map = series_base.GetSeries();
  ASSERT_EQ(series_map.size(), 5);
  ASSERT_EQ(series_map["now"][0], 59);
  ASSERT_EQ(series_map["latest_sec"][0], 59);
  ASSERT_EQ(series_map["latest_sec"][1], 58);
  ASSERT_EQ(series_map["latest_sec"][59], 0);
  ASSERT_EQ(series_map["latest_min"][0], 59);
  ASSERT_EQ(series_map["latest_hour"][0], 59);
  ASSERT_EQ(series_map["latest_day"][0], 59);
}

}  // namespace trpc::testing
