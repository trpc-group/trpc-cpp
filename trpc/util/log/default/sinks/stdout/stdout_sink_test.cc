#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/common/config/stdout_sink_conf.h"
#include "trpc/util/log/default/sinks/stdout/stdout_sink.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

class StdoutSinkTest : public ::testing::Test {
 public:
  void SetUp() override {
    stdout_sink_ = MakeRefCounted<StdoutSink>();
    ASSERT_TRUE(stdout_sink_ != nullptr);
  }

  StdoutSinkPtr stdout_sink_;
};

TEST_F(StdoutSinkTest, InitTest) {
  StdoutSinkConfig config;
  config.stderr_level = 2;
  config.eol = true;
  config.format = "[%Y-%m-%d %H:%M:%S.%e] [%l] %v";

  ASSERT_EQ(stdout_sink_->Init(config), 0);
  ASSERT_TRUE(stdout_sink_->SpdSink() != nullptr);
}

TEST_F(StdoutSinkTest, LogTest) {
  // It's difficult to directly test the log output of the StdoutSink
  // because it writes to stdout and stderr. Instead, we can check if the
  // log function is callable without any issues.

  // Initialize the sink
  StdoutSinkConfig config;
  config.stderr_level = 2;
  config.eol = true;
  config.format = "[%Y-%m-%d %H:%M:%S.%e] [%l] %v";
  ASSERT_EQ(stdout_sink_->Init(config), 0);

  // Create a log message
  spdlog::details::log_msg log_msg;
  log_msg.level = spdlog::level::info;
  log_msg.logger_name = "test_logger";
  log_msg.time = spdlog::log_clock::now();
  log_msg.source = spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__};
  log_msg.payload = "Test log message";

  // Call the log function
  stdout_sink_->SpdSink()->log(log_msg);
}

}  // namespace trpc