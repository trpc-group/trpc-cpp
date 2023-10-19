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

#include "trpc/util/log/logging.h"

#include <memory>
#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "spdlog/async.h"
#include "spdlog/spdlog.h"

#include "trpc/common/config/config_helper.h"
#include "trpc/util/log/default/default_log.h"
#include "trpc/util/log/default/testing/mock_sink.h"
#include "trpc/util/time.h"

namespace trpc::testing {

class LoggingTest : public ::testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(ConfigHelper::GetInstance()->Init("trpc/util/log/testing/trpc_log_test.yaml"));
    ASSERT_TRUE(trpc::log::Init());
    auto log = LogFactory::GetInstance()->Get();
    ASSERT_NE(log, nullptr);

    default_log_ = static_pointer_cast<DefaultLog>(log);
    ASSERT_NE(default_log_, nullptr);

    auto logger = default_log_->GetLoggerInstance("default")->logger;
    ASSERT_NE(logger, nullptr);

    // Create a MockSink and add it to the logger
    mock_sink_ = std::make_shared<MockSink::Sink>();
    logger->sinks().push_back(mock_sink_);
  }

  void TearDown() override { trpc::log::Destroy(); }

  std::shared_ptr<MockSink::Sink> mock_sink_;
  DefaultLogPtr default_log_;
};

TEST_F(LoggingTest, TRPC_FMT_Test) {
  using namespace ::testing;

  // now log level is debug
  EXPECT_CALL(*mock_sink_, log(_)).Times(5).WillRepeatedly(Invoke([](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    EXPECT_THAT(log_msg, HasSubstr("TRPC_FMT_Test"));
  }));

  TRPC_FMT_TRACE("TRPC_FMT_Test");
  TRPC_FMT_DEBUG("TRPC_FMT_Test");
  TRPC_FMT_INFO("TRPC_FMT_Test");
  TRPC_FMT_WARN("TRPC_FMT_Test");
  TRPC_FMT_ERROR("TRPC_FMT_Test");
  TRPC_FMT_CRITICAL("TRPC_FMT_Test");
}

TEST_F(LoggingTest, TRPC_PRT_Test) {
  using namespace ::testing;

  // Change log level to err
  default_log_->SetLevel("default", Log::trace);

  EXPECT_CALL(*mock_sink_, log(_)).Times(6).WillRepeatedly(Invoke([](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    EXPECT_THAT(log_msg, HasSubstr("TRPC_PRT_Test"));
  }));

  TRPC_PRT_TRACE("TRPC_PRT_Test");
  TRPC_PRT_DEBUG("TRPC_PRT_Test");
  TRPC_PRT_INFO("TRPC_PRT_Test");
  TRPC_PRT_WARN("TRPC_PRT_Test");
  TRPC_PRT_ERROR("TRPC_PRT_Test");
  TRPC_PRT_CRITICAL("TRPC_PRT_Test");
}

TEST_F(LoggingTest, TRPC_Log_Test) {
  using namespace ::testing;

  // Change log level to info
  default_log_->SetLevel("default", Log::info);

  EXPECT_CALL(*mock_sink_, log(_)).Times(4).WillRepeatedly(Invoke([](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    EXPECT_THAT(log_msg, HasSubstr("TRPC_Log_Test"));
  }));

  TRPC_LOG_TRACE("TRPC_Log_Test");
  TRPC_LOG_DEBUG("TRPC_Log_Test");
  TRPC_LOG_INFO("TRPC_Log_Test");
  TRPC_LOG_WARN("TRPC_Log_Test");
  TRPC_LOG_ERROR("TRPC_Log_Test");
  TRPC_LOG_CRITICAL("TRPC_Log_Test");
}

TEST_F(LoggingTest, TRPC_FMT_ONCE_Test) {
  using namespace ::testing;

  EXPECT_CALL(*mock_sink_, log(_)).Times(1).WillOnce(Invoke([](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    EXPECT_THAT(log_msg, HasSubstr("TRPC_FMT_ONCE_Test"));
  }));

  for (int i = 0; i < 5; ++i) {
    TRPC_FMT_INFO_ONCE("TRPC_FMT_ONCE_Test");
  }
}

TEST_F(LoggingTest, TRPC_FMT_EVERY_N_Test) {
  using namespace ::testing;

  EXPECT_CALL(*mock_sink_, log(_)).Times(5).WillRepeatedly(Invoke([](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    EXPECT_THAT(log_msg, HasSubstr("TRPC_FMT_EVERY_N_Test"));
  }));

  for (int i = 0; i < 25; ++i) {
    TRPC_FMT_INFO_EVERY_N(5, "TRPC_FMT_EVERY_N_Test");
  }
}

TEST_F(LoggingTest, TRPC_FMT_EVERY_SECOND_Test) {
  using namespace ::testing;

  EXPECT_CALL(*mock_sink_, log(_)).WillRepeatedly(Invoke([](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    EXPECT_THAT(log_msg, HasSubstr("TRPC_FMT_EVERY_SECOND_Test"));
  }));

  auto start_time = std::chrono::steady_clock::now();
  while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_time).count() < 3) {
    TRPC_FMT_INFO_EVERY_SECOND("TRPC_FMT_EVERY_SECOND_Test");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
}

}  // namespace trpc::testing
