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

#include "trpc/util/log/stream_like.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "spdlog/async.h"
#include "spdlog/spdlog.h"

#include "trpc/client/client_context.h"
#include "trpc/common/config/config_helper.h"
#include "trpc/server/server_context.h"
#include "trpc/util/log/default/default_log.h"
#include "trpc/util/log/default/testing/mock_sink.h"
#include "trpc/util/log/logging.h"

namespace trpc::testing {

class StreamLikeTest : public ::testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(ConfigHelper::GetInstance()->Init("trpc/util/log/testing/trpc_log_test.yaml"));
    ASSERT_TRUE(trpc::log::Init());

    auto log = LogFactory::GetInstance()->Get();
    ASSERT_NE(log, nullptr);

    DefaultLogPtr default_log = static_pointer_cast<DefaultLog>(log);
    ASSERT_NE(default_log, nullptr);

    auto logger = default_log->GetLoggerInstance("default")->logger;
    ASSERT_NE(logger, nullptr);

    // Create a MockSink and add it to the logger
    mock_sink_ = std::make_shared<MockSink::Sink>();
    logger->sinks().push_back(mock_sink_);
  }

  void TearDown() override { trpc::log::Destroy(); }

  std::shared_ptr<MockSink::Sink> mock_sink_;
};

TEST_F(StreamLikeTest, TRPC_LOG_MSG_Test) {
  using namespace ::testing;

  EXPECT_CALL(*mock_sink_, log(_)).Times(1).WillOnce(Invoke([](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    EXPECT_THAT(log_msg, HasSubstr("TRPC_LOG_MSG_Test"));
  }));

  TRPC_LOG_MSG(Log::Level::info, "TRPC_LOG_MSG_Test");
}

TEST_F(StreamLikeTest, TRPC_LOG_MSG_IF_Test) {
  using namespace ::testing;
  bool condition = true;

  EXPECT_CALL(*mock_sink_, log(_)).Times(1).WillOnce(Invoke([](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    EXPECT_THAT(log_msg, HasSubstr("TRPC_LOG_MSG_IF_Test"));
  }));

  TRPC_LOG_MSG_IF(Log::Level::info, condition, "TRPC_LOG_MSG_IF_Test");
}

TEST_F(StreamLikeTest, TRPC_LOG_MSG_EX_Test) {
  using namespace ::testing;

  EXPECT_CALL(*mock_sink_, log(_)).Times(1).WillOnce(Invoke([](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    EXPECT_THAT(log_msg, HasSubstr("TRPC_LOG_MSG_EX_Test"));
  }));

  ClientContextPtr client_context = MakeRefCounted<ClientContext>();
  TRPC_LOG_MSG_EX(Log::Level::info, client_context, "TRPC_LOG_MSG_EX_Test");
}

TEST_F(StreamLikeTest, TRPC_LOG_MSG_IF_EX_Test) {
  using namespace ::testing;
  bool condition = true;

  EXPECT_CALL(*mock_sink_, log(_)).Times(1).WillOnce(Invoke([](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    EXPECT_THAT(log_msg, HasSubstr("TRPC_LOG_MSG_IF_EX_Test"));
  }));

  ClientContextPtr client_context = MakeRefCounted<ClientContext>();
  TRPC_LOG_MSG_IF_EX(Log::Level::info, client_context, condition, "TRPC_LOG_MSG_IF_EX_Test");
}

TEST_F(StreamLikeTest, TRPC_LOGGER_MSG_EX_Test) {
  using namespace ::testing;

  EXPECT_CALL(*mock_sink_, log(_)).Times(1).WillOnce(Invoke([](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    EXPECT_THAT(log_msg, HasSubstr("TRPC_LOGGER_MSG_EX_Test"));
  }));

  ClientContextPtr client_context = MakeRefCounted<ClientContext>();
  TRPC_LOGGER_MSG_EX(Log::Level::info, client_context, "default", "TRPC_LOGGER_MSG_EX_Test");
}

TEST_F(StreamLikeTest, TRPC_LOGGER_MSG_IF_EX_Test) {
  using namespace ::testing;

  EXPECT_CALL(*mock_sink_, log(_)).Times(1).WillOnce(Invoke([](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    EXPECT_THAT(log_msg, HasSubstr("TRPC_LOGGER_MSG_IF_EX_Test"));
  }));

  bool condition = true;
  ClientContextPtr client_context = MakeRefCounted<ClientContext>();
  TRPC_LOGGER_MSG_IF_EX(Log::Level::info, client_context, "default", condition, "TRPC_LOGGER_MSG_IF_EX_Test");
}

}  // namespace trpc::testing
