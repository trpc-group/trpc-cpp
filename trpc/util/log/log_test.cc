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

#include "trpc/util/log/log.h"
#include "trpc/util/log/testing/mock_log.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::testing {

class LogFactoryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    log_factory_ = trpc::LogFactory::GetInstance();
    ASSERT_TRUE(log_factory_ != nullptr);
  }

  trpc::LogFactory* log_factory_;
};

TEST_F(LogFactoryTest, RegisterAndGetAndReset) {
  // Create a MockLog instance and register it with LogFactory.
  trpc::LogPtr log = LogFactory::GetInstance()->Get();
  auto mock_log = static_pointer_cast<MockLog>(log);
  log_factory_->Register(mock_log);

  // Verify that the registered log instance is the same as the one we created.
  trpc::LogPtr retrieved_log = log_factory_->Get();
  EXPECT_EQ(retrieved_log.get(), mock_log.get());

  // Reset the LogFactory and verify that the log instance is no longer available.
  log_factory_->Reset();
  trpc::LogPtr empty_log = log_factory_->Get();
  EXPECT_EQ(empty_log.get(), nullptr);
}

}  // namespace trpc::testing
