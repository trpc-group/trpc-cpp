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

#include "trpc/util/log/default/default_log.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/common/config/config_helper.h"
#include "trpc/util/log/default/testing/mock_sink.h"
#include "trpc/util/log/logging.h"

namespace trpc::testing {

class DefaultLogTest : public ::testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(ConfigHelper::GetInstance()->Init("trpc/util/log/default/testing/default_log_test.yaml"));
    ASSERT_TRUE(trpc::log::Init());

    auto log = LogFactory::GetInstance()->Get();
    ASSERT_NE(log, nullptr);

    default_log_ = static_pointer_cast<DefaultLog>(log);
    ASSERT_NE(default_log_, nullptr);
  }

  void TearDown() override { trpc::log::Destroy(); }

  DefaultLogPtr default_log_;
};

// Test case for ShouldLog method with instance_name and level
TEST_F(DefaultLogTest, ShouldLogTestWithInstanceNameAndLevel) {
  const char* kInstance = "default";
  Log::Level level = Log::info;

  bool should_log = default_log_->ShouldLog(kInstance, level);
  ASSERT_TRUE(should_log);
}

// Test case for ShouldLog method with level
TEST_F(DefaultLogTest, ShouldLogTestWithLevel) {
  Log::Level level = Log::info;

  bool should_log = default_log_->ShouldLog(level);
  ASSERT_TRUE(should_log);
}

// Test case for GetLevel method
TEST_F(DefaultLogTest, GetLevelTest) {
  const char* kInstance = "default";
  auto level_info = default_log_->GetLevel(kInstance);

  ASSERT_TRUE(level_info.second);
  ASSERT_EQ(level_info.first, Log::info);
}

// Test case for SetLevel method
TEST_F(DefaultLogTest, SetLevelTest) {
  const char* kInstance = "default";
  Log::Level new_level = Log::debug;

  auto level_info = default_log_->SetLevel(kInstance, new_level);

  ASSERT_TRUE(level_info.second);
  ASSERT_EQ(default_log_->GetLevel(kInstance).first, new_level);
}

// Test case for LogIt method with default instance_name
TEST_F(DefaultLogTest, LogItTestWithDefaultInstanceName) {
  const char* kInstance = kTrpcLogCacheStringDefault;
  Log::Level level = Log::info;
  const char* filename = "test_file";
  int line = 42;
  const char* funcname = "test_func";
  std::string msg = "Test message";

  default_log_->LogIt(kInstance, level, filename, line, funcname, msg);
}

// Test case for LogIt method with custom instance_name
TEST_F(DefaultLogTest, LogItTestWithCustomInstanceName) {
  const char* kInstance = "custom_instance";
  Log::Level level = Log::info;
  const char* filename = "test_file";
  int line = 42;
  const char* funcname = "test_func";
  std::string msg = "Test message";

  default_log_->LogIt(kInstance, level, filename, line, funcname, msg);
}

}  // namespace trpc::testing
