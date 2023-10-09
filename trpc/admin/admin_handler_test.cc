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

#include "trpc/admin/admin_handler.h"

#include <memory>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/server/server_context.h"
#include "trpc/util/http/mime_types.h"

namespace trpc::testing {

constexpr char kAdminDescription[] = "test admin handler description";

class TestHandler : public AdminHandlerBase {
 public:
  TestHandler() { description_ = kAdminDescription; }
  ~TestHandler() override = default;
  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override {
    result.AddMember("message", "test", alloc);
  }
};

class TestWebHandler : public AdminHandlerBase {
 public:
  TestWebHandler() { description_ = kAdminDescription; }

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result, rapidjson::Document::AllocatorType& alloc) {
    result.AddMember("trpc-html", "alert('get commands fail, please check')", alloc);
  }
};

class TestAdminHandler : public ::testing::Test {
 public:
};

TEST_F(TestAdminHandler, CheckDescription) {
  std::unique_ptr<AdminHandlerBase> h1(new TestHandler);
  EXPECT_EQ(kAdminDescription, h1->Description());
}

TEST_F(TestAdminHandler, CheckReply) {
  std::unique_ptr<AdminHandlerBase> h1 = std::make_unique<TestHandler>();
  http::HttpRequestPtr req = std::make_shared<http::HttpRequest>();
  http::HttpResponse reply;
  ServerContextPtr context;
  auto status = h1->Handle("", context, req, &reply);
  // check fuc ret
  EXPECT_TRUE(status.OK());
  // check content-type
  EXPECT_TRUE(!reply.GetHeader("Content-Type").empty());
  EXPECT_EQ(reply.GetHeader("Content-Type"), http::ExtensionToType("json"));
  // check body
  EXPECT_EQ("{\"message\":\"test\"}", reply.GetContent());

  std::unique_ptr<AdminHandlerBase> h2 = std::make_unique<TestWebHandler>();
  EXPECT_EQ(kAdminDescription, h2->Description());

  http::HttpRequestPtr req2 = std::make_shared<http::HttpRequest>();
  http::HttpResponse reply2;
  h2->Handle("", context, req2, &reply2);
  EXPECT_EQ("alert('get commands fail, please check')", reply2.GetContent());
}

}  // namespace trpc::testing
