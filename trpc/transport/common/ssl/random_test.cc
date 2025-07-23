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

#include "trpc/transport/common/ssl/random.h"

#include <vector>

#include "gtest/gtest.h"

namespace trpc::testing {

// For testing purposes only.
using namespace trpc::ssl;

class RandomTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(RandomTest, RandomInt) {
  int begin = 0;
  int end = 1000000;
  int cycles = 1000000;
  for (int i = 0; i < cycles; ++i) {
    int r_i = Random::RandomInt(begin, end);
    EXPECT_LE(r_i, end);
    EXPECT_GE(r_i, begin);
  }
}

TEST_F(RandomTest, RandomIntWithDifferentRange) {
  struct testing_args_t {
    int begin{0};
    int end{100};
  };

  std::vector<testing_args_t> testings{
      {0, 10},
      {10, 20},
      {100, 200},
      {300, 100000},
  };

  for (const auto& t : testings) {
    int r_i = Random::RandomInt(t.begin, t.end);
    EXPECT_LE(r_i, t.end);
    EXPECT_GE(r_i, t.begin);
  }
}

}  // namespace trpc::testing
