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

#include "trpc/filter/filter_base.h"

#include <string>

#include "gtest/gtest.h"

namespace trpc::testing {

class FilterString : public Filter<std::string> {
 public:
  std::string Name() override { return "FilterString"; }

  std::vector<FilterPoint> GetFilterPoint() override {
    static std::vector<FilterPoint> filter_points = {FilterPoint::CLIENT_PRE_RPC_INVOKE};
    return filter_points;
  }

  void operator()(FilterStatus& status, FilterPoint point, std::string&& arg) override {
    // ...
  }
};

class FilterInt : public Filter<int> {
 public:
  std::string Name() override { return "FilterInt"; }

  std::vector<FilterPoint> GetFilterPoint() override {
    static std::vector<FilterPoint> filter_points = {FilterPoint::CLIENT_POST_RPC_INVOKE};
    return filter_points;
  }

  void operator()(FilterStatus& status, FilterPoint point, int&& arg) override {
    // ...
  }
};

TEST(FilterTest, TestFilterID) {
  FilterString filter_string;
  FilterInt filter_int;

  uint32_t filter_string_id = filter_string.GetFilterID(); // 10000
  uint32_t filter_int_id = filter_int.GetFilterID(); // 10001

  EXPECT_EQ(filter_string_id + 1, filter_int_id);
}

}  // namespace trpc::testing
