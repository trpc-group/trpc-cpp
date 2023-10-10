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

#include <gtest/gtest.h>

#include "trpc/server/server_context.h"

#define private public
#define protected public

#include "trpc/util/http/http_handler_groups.h"

namespace trpc::testing {

class TestHandler : public trpc::http::HttpHandler {
 public:
  trpc::Status Handle(const trpc::ServerContextPtr&, const trpc::http::RequestPtr& req,
                      trpc::http::Response* rsp) override {
    rsp->Done("json");
    return trpc::kSuccStatus;
  }

  using trpc::http::HttpHandler::Handle;
};

class HttpHandlerGroupsTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() { routes_ = new trpc::http::Routes(); }

  static void TearDownTestCase() {
    delete routes_;
    routes_ = nullptr;
  }

  static trpc::http::Routes* routes_;

  static std::shared_ptr<TestHandler> h1, h2, h3, h4, h5, h6;
};

trpc::http::Routes* HttpHandlerGroupsTest::routes_ = nullptr;
std::shared_ptr<TestHandler> HttpHandlerGroupsTest::h1 = std::make_shared<TestHandler>();
std::shared_ptr<TestHandler> HttpHandlerGroupsTest::h2 = std::make_shared<TestHandler>();
std::shared_ptr<TestHandler> HttpHandlerGroupsTest::h3 = std::make_shared<TestHandler>();
std::shared_ptr<TestHandler> HttpHandlerGroupsTest::h4 = std::make_shared<TestHandler>();
std::shared_ptr<TestHandler> HttpHandlerGroupsTest::h5 = std::make_shared<TestHandler>();
std::shared_ptr<TestHandler> HttpHandlerGroupsTest::h6 = std::make_shared<TestHandler>();

class TestUserController {
 public:
  trpc::Status GetUser(const trpc::ServerContextPtr& ctx, const trpc::http::RequestPtr& req,
                       trpc::http::Response* rsp) {
    // do stuff here
    return trpc::kSuccStatus;
  }

  trpc::Status Download(const trpc::ServerContextPtr& ctx, const trpc::http::RequestPtr& req,
                        trpc::http::Response* rsp) {
    // do stuff here
    return trpc::kSuccStatus;
  }
};

TEST_F(HttpHandlerGroupsTest, TestHttpHandlerGroups) {
  auto h7 = std::make_shared<TestHandler>();
  // clang-format off
  http::HttpHandlerGroups(*routes_).Path("/", TRPC_HTTP_ROUTE_HANDLER_ARGS((h7), (auto h7){
    auto user_controller = std::make_shared<TestUserController>();
    Get("/test", h1);
    Get("/user", TRPC_HTTP_HANDLER(user_controller, TestUserController::GetUser));
    Get("/user_r", TRPC_HTTP_HANDLER(std::make_shared<TestUserController>(), TestUserController::GetUser));
    Get("/download", TRPC_HTTP_STREAM_HANDLER(std::make_shared<TestUserController>(), TestUserController::Download));
    Path("/api", TRPC_HTTP_ROUTE_HANDLER({
      Get("/teapot", [](auto ctx, auto req, auto rsp) {  // the first match takes priority
        rsp->SetStatus(http::Response::StatusCode::kIAmATeapot);
        return trpc::kSuccStatus;
      });
      Get("<path>", h2);
    }));
    Path("/hello", TRPC_HTTP_ROUTE_HANDLER_ARGS((h7), (auto h7){
      Post(h3);  // the first match takes priority
      Post("/", h5);
      Patch(h7);
    }));
    Post("/api", h4);
    Get<TestHandler>("/auto_construct");
    Get("/channels/<channel_id>/clients/<client_id>", h6);
  }));
  // clang-format on

  trpc::http::Parameters params;
  auto h = routes_->GetHandler(trpc::http::MethodType::POST, "/hello", params);
  ASSERT_EQ(h3.get(), h);

  h = routes_->GetHandler(trpc::http::MethodType::GET, "/user", params);
  EXPECT_NE(nullptr, h);
  h = routes_->GetHandler(trpc::http::MethodType::GET, "/user_r", params);
  EXPECT_NE(nullptr, h);
  h = routes_->GetHandler(trpc::http::MethodType::GET, "/download", params);
  EXPECT_NE(nullptr, h);

  h = routes_->GetHandler(trpc::http::MethodType::POST, "/api", params);
  ASSERT_EQ(h4.get(), h);

  h = routes_->GetHandler(trpc::http::MethodType::GET, "/test1", params);
  ASSERT_EQ(nullptr, h);

  h = routes_->GetHandler(trpc::http::MethodType::PUT, "/test", params);
  ASSERT_EQ(nullptr, h);

  h = routes_->GetHandler(trpc::http::MethodType::GET, "/test", params);
  ASSERT_EQ(h1.get(), h);

  h = routes_->GetHandler(trpc::http::MethodType::GET, "/api/hello", params);
  ASSERT_EQ(h2.get(), h);
  ASSERT_EQ("hello", params.At("path"));

  h = routes_->GetHandler(trpc::http::MethodType::GET, "/channels/abc/clients/123", params);
  ASSERT_EQ(h6.get(), h);
  ASSERT_EQ("abc", params.Path("channel_id"));
  ASSERT_EQ("123", params.Path("client_id"));

  h = routes_->GetHandler(trpc::http::MethodType::PATCH, "/hello", params);
  ASSERT_EQ(h7.get(), h);

  h = routes_->GetHandler(trpc::http::MethodType::GET, "/auto_construct", params);
  EXPECT_NE(nullptr, h);

  auto req1 = std::make_shared<trpc::http::Request>();
  auto req2 = std::make_shared<trpc::http::Request>();
  auto req3 = std::make_shared<trpc::http::Request>();

  ServerContextPtr context;
  {
    trpc::http::Response reply;
    routes_->Handle("/test1", context, req1, reply);
    ASSERT_EQ(trpc::http::ResponseStatus::kNotFound, reply.GetStatus());
  }
  {
    trpc::http::Response reply;
    routes_->Handle("/download", context, req1, reply);
    ASSERT_EQ(trpc::http::ResponseStatus::kOk, reply.GetStatus());
  }

  {
    trpc::http::Response reply;
    req2->SetHeader("Connection", "Keep-Alive");
    routes_->Handle("/api/123", context, req2, reply);
    ASSERT_EQ(trpc::http::ResponseStatus::kOk, reply.GetStatus());
  }

  {
    trpc::http::Response reply;
    req3->SetMethodType(http::GET);
    routes_->Handle("/api/teapot", context, req3, reply);
    ASSERT_EQ(trpc::http::ResponseStatus::kIAmATeapot, reply.GetStatus());
  }
  {
    trpc::http::Response reply;
    req3->SetMethodType(http::POST);
    routes_->Handle("/api/teapot", context, req3, reply);
    ASSERT_EQ(trpc::http::ResponseStatus::kNotFound, reply.GetStatus());
  }
}

TEST_F(HttpHandlerGroupsTest, TestNormalizePath) {
  http::HttpHandlerGroups handler_groups(*routes_);
  ASSERT_EQ("", handler_groups.basic_path_);
  ASSERT_EQ("", handler_groups.NormalizePath(""));
  ASSERT_EQ("", handler_groups.NormalizePath("/"));
  ASSERT_EQ("/teapot", handler_groups.NormalizePath("/teapot"));
  ASSERT_EQ("/teapot", handler_groups.NormalizePath("teapot"));
  ASSERT_EQ("/teapot", handler_groups.NormalizePath("teapot/"));
  ASSERT_EQ("/teapot", handler_groups.NormalizePath("/teapot/"));

  http::HttpHandlerGroups handler_groups_child(handler_groups, "/api");
  ASSERT_EQ("/api", handler_groups_child.basic_path_);
  ASSERT_EQ("/api", handler_groups_child.NormalizePath(""));
  ASSERT_EQ("/api", handler_groups_child.NormalizePath("/"));
  ASSERT_EQ("/api/user", handler_groups_child.NormalizePath("/user"));
  ASSERT_EQ("/api/user", handler_groups_child.NormalizePath("user"));
  ASSERT_EQ("/api/user", handler_groups_child.NormalizePath("user/"));
  ASSERT_EQ("/api/user", handler_groups_child.NormalizePath("/user/"));
}

TEST_F(HttpHandlerGroupsTest, TestHttpPath) {
  http::Path path = http::HttpHandlerGroups::HttpPath("/api/path");
  ASSERT_EQ("/api/path", path.GetPath());
  http::Path ph_path = http::HttpHandlerGroups::HttpPath("/api/<path>");
  ASSERT_EQ("<ph(/api/<path>)>", ph_path.GetPath());
}

}  // namespace trpc::testing
