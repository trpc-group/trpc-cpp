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

#include "trpc/util/log/python_like.h"

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

class PythonLikeTest : public ::testing::Test {
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

TEST_F(PythonLikeTest, TRPC_FMT_Test) {
  using namespace ::testing;
  std::string_view kMsg = "trpc_fmt_test";
  // Test TRPC_FMT macro
  EXPECT_CALL(*mock_sink_, log(_)).Times(1).WillOnce(Invoke([kMsg](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    EXPECT_THAT(log_msg, HasSubstr(std::string(kMsg)));
  }));

  TRPC_FMT("default", Log::Level::info, "{}", kMsg);
}

TEST_F(PythonLikeTest, TRPC_FMT_IF_Test) {
  using namespace ::testing;
  std::string_view kMsg = "trpc_fmt_if_test";
  bool condition = true;

  EXPECT_CALL(*mock_sink_, log(_)).Times(1).WillOnce(Invoke([kMsg](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    EXPECT_THAT(log_msg, HasSubstr(std::string(kMsg)));
  }));

  TRPC_FMT_IF("default", condition, Log::Level::info, "{}", kMsg);
}

TEST_F(PythonLikeTest, TRPC_FMT_EX_Test) {
  using namespace ::testing;

  EXPECT_CALL(*mock_sink_, log(_)).Times(1).WillOnce(Invoke([](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    EXPECT_THAT(log_msg, HasSubstr("TRPC_FMT_EX_Test"));
  }));

  ClientContextPtr client_context = MakeRefCounted<ClientContext>();
  TRPC_FMT_EX(client_context, "default", Log::Level::info, "TRPC_FMT_EX_Test: {}", 1);
}

TEST_F(PythonLikeTest, TRPC_FMT_IF_EX_Test) {
  using namespace ::testing;

  EXPECT_CALL(*mock_sink_, log(_)).Times(1).WillOnce(Invoke([](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    EXPECT_THAT(log_msg, HasSubstr("TRPC_FMT_IF_EX_Test"));
  }));

  ServerContextPtr server_context = MakeRefCounted<ServerContext>();
  TRPC_FMT_IF_EX(server_context, "default", true, Log::Level::info, "TRPC_FMT_IF_EX_Test: {}", 1);
}

}  // namespace trpc::testing
