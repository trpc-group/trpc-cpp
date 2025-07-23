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

#include "trpc/util/http/parameter.h"

#include "gtest/gtest.h"

namespace trpc::testing {

class ParametersTest : public ::testing::Test {
 protected:
  void SetUp() override {
    params_.Set("key_00", "value_00");
    params_.Set(std::string{"key_01"}, std::string{"value_01"});
    params_.Set(std::string{"path"}, std::string{"/home/index.html"});
  }

  void TearDown() override {}

 protected:
  trpc::http::PathParameters params_;
};

TEST_F(ParametersTest, Get) {
  ASSERT_EQ("value_00", params_.Get("key_00"));
  ASSERT_EQ("", params_.Get("key_not_exists"));
  ASSERT_EQ("default_val", params_.Get("key_not_exists", "default_val"));
  ASSERT_EQ("value_01", params_.Path("key_01"));
  ASSERT_EQ("value_01", params_.At("key_01"));
  ASSERT_EQ("home/index.html", params_["path"]);
}

TEST_F(ParametersTest, Range) {
  int counter{0};
  params_.Range([&counter](std::string_view key, std::string_view value) {
    ++counter;
    return true;
  });
  ASSERT_EQ(counter, 3);
}

TEST_F(ParametersTest, Exists) {
  ASSERT_TRUE(params_.Exists("key_00"));
  ASSERT_FALSE(params_.Exists("key_not_exists"));
}

TEST_F(ParametersTest, Clear) {
  ASSERT_EQ(3, params_.PairsCount());
  params_.Clear();
  ASSERT_EQ(0, params_.PairsCount());
}

TEST_F(ParametersTest, Delete) {
  ASSERT_EQ(3, params_.PairsCount());
  params_.Delete("key_00");
  ASSERT_EQ(2, params_.PairsCount());
  params_.Set("key_00", "value_00");
  params_.Delete("key_not_found");
  ASSERT_EQ(3, params_.PairsCount());
}


class BodyParamsTest : public ::testing::Test {
 public:
};

TEST_F(BodyParamsTest, EmptyBody) {
  http::BodyParam body_param("");
  EXPECT_EQ(body_param.Size(), 0);
}

TEST_F(BodyParamsTest, NotEmptyBody) {
  http::BodyParam body_param("a=1&b=2");
  EXPECT_EQ(body_param.Size(), 2);
  EXPECT_EQ(body_param.GetBodyParam("a"), "1");
  EXPECT_EQ(body_param.GetBodyParam("b"), "2");
}

}  // namespace trpc::testing
