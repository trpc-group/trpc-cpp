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

#include "trpc/admin/log_level_handler.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/server/server_context.h"

namespace trpc::testing {

class TestLogLevelHandler : public ::testing::Test {
 public:
  void SetUp() override {
    EXPECT_EQ(TrpcConfig::GetInstance()->Init("trpc/admin/test.yaml"), 0);
    EXPECT_TRUE(log::Init());
  }
  void TearDown() override { log::Destroy(); }

  std::map<std::string, std::string> set_log_case_{
      {"TRACE", R"({"errorcode":0,"message":"","level":"TRACE"})"},
      {"DEBUG", R"({"errorcode":0,"message":"","level":"DEBUG"})"},
      {"INFO", R"({"errorcode":0,"message":"","level":"INFO"})"},
      {"WARNING", R"({"errorcode":0,"message":"","level":"WARNING"})"},
      {"ERROR", R"({"errorcode":0,"message":"","level":"ERROR"})"},
      {"CRITICAL", R"({"errorcode":0,"message":"","level":"CRITICAL"})"},
      // if use "raw string literals", will exceed 100, so use \"
      {
          "wrong",
          "{\"errorcode\":-3,\"message\":\"wrong level, please use "
          "TRACE,DEBUG,INFO,WARNING,ERROR,CRITICAL\"}",
      },
  };
};

TEST_F(TestLogLevelHandler, CheckGetLogLevel1) {
  ServerContextPtr context;
  // Get Log Level
  std::unique_ptr<AdminHandlerBase> get_log_handler = std::make_unique<admin::LogLevelHandler>();
  {
    // default logger
    http::HttpResponse reply;
    get_log_handler->Handle("", context, std::make_shared<http::HttpRequest>(), &reply);
    std::cout << reply.GetContent() << std::endl;
    EXPECT_EQ(reply.GetContent(), R"({"errorcode":0,"message":"","level":"INFO"})");
  }
  {
    // get logger from query param
    http::HttpResponse reply;
    auto req = std::make_shared<http::HttpRequest>();
    req->AddQueryParameter("logger=test");
    get_log_handler->Handle("", context, std::move(req), &reply);
    std::cout << reply.GetContent() << std::endl;
    EXPECT_EQ(reply.GetContent(), R"({"errorcode":0,"message":"","level":"WARNING"})");
  }
  {
    // logger not exist
    http::HttpResponse reply;
    auto req = std::make_shared<http::HttpRequest>();
    req->AddQueryParameter("logger=no");
    get_log_handler->Handle("", context, std::move(req), &reply);
    std::cout << reply.GetContent() << std::endl;
    EXPECT_EQ(reply.GetContent(), R"({"errorcode":-5,"message":"get level failed, does logger exist?"})");
  }
  // Set Log Level
  std::unique_ptr<AdminHandlerBase> set_log_handler = std::make_unique<admin::LogLevelHandler>(true);
  {
    // default logger
    for (auto const& kv : set_log_case_) {
      auto req = std::make_shared<http::HttpRequest>();
      req->SetContent("value=" + kv.first);
      http::HttpResponse reply;
      set_log_handler->Handle("", context, std::move(req), &reply);
      std::cout << reply.GetContent() << std::endl;
      EXPECT_EQ(reply.GetContent(), kv.second);
    }
  }
  {
    // get logger from query param
    for (auto const& kv : set_log_case_) {
      auto req = std::make_shared<http::HttpRequest>();
      req->SetContent("value=" + kv.first);
      req->AddQueryParameter("logger=test");
      http::HttpResponse reply;
      set_log_handler->Handle("", context, std::move(req), &reply);
      std::cout << reply.GetContent() << std::endl;
      EXPECT_EQ(reply.GetContent(), kv.second);
    }
  }
  {
    // get logger from body
    for (auto const& kv : set_log_case_) {
      auto req = std::make_shared<http::HttpRequest>();
      req->SetContent("logger=test&value=" + kv.first);
      http::HttpResponse reply;
      set_log_handler->Handle("", context, std::move(req), &reply);
      std::cout << reply.GetContent() << std::endl;
      EXPECT_EQ(reply.GetContent(), kv.second);
    }
  }
  {
    // logger not exist
    auto req = std::make_shared<http::HttpRequest>();
    req->SetContent("logger=no&value=INFO");
    http::HttpResponse reply;
    set_log_handler->Handle("", context, std::move(req), &reply);
    std::cout << reply.GetContent() << std::endl;
    EXPECT_EQ(reply.GetContent(), R"({"errorcode":-4,"message":"set level failed, does logger exist?"})");
  }
}

// not use log plugin
class TestLogLevelHandlerWhitNoLogPlugin : public ::testing::Test {
 public:
  void SetUp() override { EXPECT_EQ(TrpcConfig::GetInstance()->Init("trpc/admin/test.yaml"), 0); }
};

TEST_F(TestLogLevelHandlerWhitNoLogPlugin, NoLog) {
  http::HttpResponse reply1;
  ServerContextPtr context;

  std::unique_ptr<AdminHandlerBase> h1 = std::make_unique<admin::LogLevelHandler>();
  h1->Handle("", context, std::make_shared<http::HttpRequest>(), &reply1);
  std::cout << reply1.GetContent() << std::endl;
  EXPECT_EQ(reply1.GetContent(), R"({"errorcode":-1,"message":"get log plugin failed, no log plugin?"})");

  http::HttpResponse reply2;
  std::unique_ptr<AdminHandlerBase> h2 = std::make_unique<admin::WebLogLevelHandler>();
  h2->Handle("", context, std::make_shared<http::HttpRequest>(), &reply2);
  EXPECT_GE(reply2.GetContent().size(), 0);
}

}  // namespace trpc::testing
