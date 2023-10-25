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

#include <any>
#include <map>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "trpc/config/testing/mock_config.h"

namespace trpc::testing {

class AnyEqMatcher : public ::testing::MatcherInterface<const std::any&> {
 public:
  explicit AnyEqMatcher(const std::any& expected) : expected_(expected) {}

  bool MatchAndExplain(const std::any& actual, ::testing::MatchResultListener* listener) const override {
    if (!actual.has_value() && !expected_.has_value()) {
      return true;
    }

    if (actual.type() != expected_.type()) {
      return false;
    }

    if (actual.type() == typeid(int)) {
      return std::any_cast<int>(actual) == std::any_cast<int>(expected_);
    } else if (actual.type() == typeid(std::string)) {
      return std::any_cast<std::string>(actual) == std::any_cast<std::string>(expected_);
    }

    return false;
  }

  void DescribeTo(::std::ostream* os) const override {
    *os << "is equal to the expected value";
  }

  void DescribeNegationTo(::std::ostream* os) const override {
    *os << "is not equal to the expected value";
  }

 private:
  const std::any expected_;
};

inline ::testing::Matcher<const std::any&> AnyEq(const std::any& expected) {
  return ::testing::MakeMatcher(new AnyEqMatcher(expected));
}

class TestConfig : public ::testing::Test {
 protected:
  void SetUp() override {
    mock_config_ = MakeRefCounted<MockConfig>();
    ASSERT_TRUE(mock_config_ != nullptr);
  }

  RefPtr<MockConfig> mock_config_;
};

TEST_F(TestConfig, PullFileConfig) {
  std::string config_name = "test_config";
  std::string config;
  std::any params;
  EXPECT_CALL(*mock_config_, PullFileConfig(config_name, &config, AnyEq(params)))
      .WillOnce(::testing::Return(true));
  bool result = mock_config_->PullFileConfig(config_name, &config, params);

  ASSERT_TRUE(result);
}

TEST_F(TestConfig, PullKVConfig) {
  std::string config_name = "test_config";
  std::string key = "test_key";
  std::string config;
  std::any params;
  EXPECT_CALL(*mock_config_, PullKVConfig(config_name, key, &config, AnyEq(params)))
      .WillOnce(::testing::Return(true));
  bool result = mock_config_->PullKVConfig(config_name, key, &config, params);

  ASSERT_TRUE(result);
}

TEST_F(TestConfig, PullMultiKVConfig) {
  std::string config_name = "test_config";
  std::map<std::string, std::string> config;
  std::any params;
  EXPECT_CALL(*mock_config_, PullMultiKVConfig(config_name, &config, AnyEq(params)))
      .WillOnce(::testing::Return(true));
  bool result = mock_config_->PullMultiKVConfig(config_name, &config, params);

  ASSERT_TRUE(result);
}

}  // namespace trpc::testing
