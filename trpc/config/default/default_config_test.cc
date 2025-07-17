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

#include "trpc/config/default/default_config.h"

#include "gtest/gtest.h"

namespace trpc::testing {

class TestDefaultConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
    default_config_ = MakeRefCounted<config::DefaultConfig>();
    ASSERT_TRUE(default_config_ != nullptr);

    test_decode_data_ = {{"key_uint32", uint32_t(42u)},
                         {"key_int32", int32_t(-42)},
                         {"key_uint64", uint64_t(123456u)},
                         {"key_int64", int64_t(-123456)},
                         {"key_float", float(3.14f)},
                         {"key_double", double(6.28)},
                         {"key_bool", true},
                         {"key_string", std::string("test_value")}};
    default_config_->SetDecodeData(test_decode_data_);
  }

  config::DefaultConfigPtr default_config_;
  std::unordered_map<std::string, std::any> test_decode_data_;
};

TEST_F(TestDefaultConfigTest, GetMethods) {
  EXPECT_EQ(default_config_->GetUint32("key_uint32", 0), 42u);
  EXPECT_EQ(default_config_->GetInt32("key_int32", 0), -42);
  EXPECT_EQ(default_config_->GetUint64("key_uint64", 0), 123456u);
  EXPECT_EQ(default_config_->GetInt64("key_int64", 0), -123456);
  EXPECT_FLOAT_EQ(default_config_->GetFloat("key_float", 0.0f), 3.14f);
  EXPECT_DOUBLE_EQ(default_config_->GetDouble("key_double", 0.0), 6.28);
  EXPECT_EQ(default_config_->GetBool("key_bool", false), true);
  EXPECT_EQ(default_config_->GetString("key_string", ""), "test_value");
}

TEST_F(TestDefaultConfigTest, DefaultValueCheck) {
  EXPECT_EQ(default_config_->GetUint32("non_existent_key", 12), 12u);
  EXPECT_EQ(default_config_->GetInt32("non_existent_key", -34), -34);
  EXPECT_EQ(default_config_->GetUint64("non_existent_key", 56), 56u);
  EXPECT_EQ(default_config_->GetInt64("non_existent_key", -78), -78);
  EXPECT_FLOAT_EQ(default_config_->GetFloat("non_existent_key", 9.1f), 9.1f);
  EXPECT_DOUBLE_EQ(default_config_->GetDouble("non_existent_key", 6.9), 6.9);
  EXPECT_EQ(default_config_->GetBool("non_existent_key", false), false);
  EXPECT_EQ(default_config_->GetString("non_existent_key", "default_string"), "default_string");
}

TEST_F(TestDefaultConfigTest, IsSetCheck) {
  EXPECT_TRUE(default_config_->IsSet("key_bool"));
  EXPECT_FALSE(default_config_->IsSet("non_existent_key"));
}

}  // namespace trpc::testing
