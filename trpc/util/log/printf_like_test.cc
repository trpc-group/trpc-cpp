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

#include "trpc/util/log/printf_like.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/client_context.h"
#include "trpc/common/config/config_helper.h"
#include "trpc/server/server_context.h"
#include "trpc/util/log/default/default_log.h"
#include "trpc/util/log/default/testing/mock_sink.h"
#include "trpc/util/log/logging.h"

#include "spdlog/async.h"
#include "spdlog/spdlog.h"

namespace trpc::testing {

class PrintfLikeTest : public ::testing::Test {
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

TEST_F(PrintfLikeTest, TRPC_PRT_Test) {
  using namespace ::testing;
  const char* kInstance = "default";
  Log::Level level = Log::info;
  std::string kFormat = "trpc_prt_test: %d, %s";
  int arg1 = 42;
  std::string arg2 = "hello";

  EXPECT_CALL(*mock_sink_, log(_)).Times(1).WillOnce(Invoke([kFormat, arg1, arg2](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    std::string expected_msg = fmt::sprintf(kFormat, arg1, arg2);
    EXPECT_THAT(log_msg, HasSubstr(expected_msg));
  }));

  TRPC_PRT(kInstance, level, kFormat.c_str(), arg1, arg2.c_str());
}

TEST_F(PrintfLikeTest, TRPC_PRT_IF_Test) {
  using namespace ::testing;
  const char* kInstance = "default";
  Log::Level level = Log::info;
  std::string kFormat = "trpc_prt_if_test: %d, %s";
  int arg1 = 42;
  std::string arg2 = "hello";
  bool condition = true;

  EXPECT_CALL(*mock_sink_, log(_)).Times(1).WillOnce(Invoke([kFormat, arg1, arg2](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    std::string expected_msg = fmt::sprintf(kFormat, arg1, arg2);
    EXPECT_THAT(log_msg, HasSubstr(expected_msg));
  }));

  TRPC_PRT_IF(kInstance, condition, level, kFormat.c_str(), arg1, arg2.c_str());
}

TEST_F(PrintfLikeTest, TRPC_PRT_EX_Test) {
  using namespace ::testing;

  EXPECT_CALL(*mock_sink_, log(_)).Times(1).WillOnce(Invoke([](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    EXPECT_THAT(log_msg, HasSubstr("TRPC_PRT_EX_Test"));
  }));

  ClientContextPtr client_context = MakeRefCounted<ClientContext>();
  TRPC_PRT_EX(client_context, "default", Log::Level::info, "TRPC_PRT_EX_Test: %d", 1);
}

TEST_F(PrintfLikeTest, TRPC_PRT_IF_EX_Test) {
  using namespace ::testing;

  EXPECT_CALL(*mock_sink_, log(_)).Times(1).WillOnce(Invoke([](const spdlog::details::log_msg& msg) {
    std::string log_msg(msg.payload.data(), msg.payload.size());
    EXPECT_THAT(log_msg, HasSubstr("TRPC_PRT_IF_EX_Test"));
  }));

  ServerContextPtr server_context = MakeRefCounted<ServerContext>();
  TRPC_PRT_IF_EX(server_context, "default", true, Log::Level::info, "TRPC_PRT_IF_EX_Test: %d", 1);
}

}  // namespace trpc::testing
