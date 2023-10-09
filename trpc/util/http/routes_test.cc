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

#include "trpc/util/http/routes.h"

#include "gtest/gtest.h"

#include "trpc/util/http/exception.h"

namespace trpc::testing {

class TestHandler : public trpc::http::HandlerBase {
 public:
  virtual trpc::Status Handle(const std::string& path, trpc::ServerContextPtr context, trpc::http::RequestPtr req,
                              trpc::http::Response* rep) {
    rep->Done("json");
    return kDefaultStatus;
  }
};

class HttpRoutesTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() { routes_ = new trpc::http::HttpRoutes(); }

  static void TearDownTestCase() {
    delete routes_;
    routes_ = nullptr;
  }

  static trpc::http::Routes* routes_;
};

trpc::http::Routes* HttpRoutesTest::routes_ = nullptr;

TEST_F(HttpRoutesTest, AddAndPut) {
  auto h0 = std::make_shared<TestHandler>();
  auto h1 = std::make_shared<TestHandler>();
  auto h2 = std::make_shared<TestHandler>();
  auto h3 = std::make_shared<TestHandler>();
  auto h4 = std::make_shared<TestHandler>();
  auto h5 = std::make_shared<TestHandler>();
  auto h6 = std::make_shared<TestHandler>();
  auto h7 = std::make_shared<TestHandler>();

  // Matches all.
  routes_->Add(trpc::http::MethodType::GET, trpc::http::Path(""), h0);
  routes_->Add(trpc::http::MethodType::GET, trpc::http::Path("/test"), h1);
  routes_->Add(trpc::http::MethodType::GET, trpc::http::Path("/api").Remainder("path"), h2);
  routes_->Put(trpc::http::MethodType::POST, "/hello", h3);
  routes_->Put(trpc::http::MethodType::POST, "/api", h4);
  routes_->Put(trpc::http::MethodType::POST, "/hello", h5);
  routes_->Put(trpc::http::MethodType::POST, "/no-double-free", h5);
  routes_->Add(trpc::http::MethodType::GET, trpc::http::Path("<ph(/channels/<channel_id>/clients/<client_id>)>"), h6);
  routes_->Add(trpc::http::MethodType::PATCH, trpc::http::Path("/hello"), h7);
  routes_->Add(trpc::http::MethodType::PATCH, trpc::http::Path("/no-double-free"), h7);

  auto h = routes_->GetExactMatch(trpc::http::MethodType::POST, "/hello");
  ASSERT_EQ(h5.get(), h);

  h = routes_->GetExactMatch(trpc::http::MethodType::POST, "/api");
  ASSERT_EQ(h4.get(), h);

  h = routes_->GetExactMatch(trpc::http::MethodType::GET, "/test1");
  ASSERT_EQ(nullptr, h);

  http::RequestPtr http_requst = std::make_shared<http::Request>();
  http_requst->SetMethodType(trpc::http::MethodType::GET);
  h = routes_->GetHandler("/", http_requst);
  ASSERT_EQ(h0.get(), h);

  http_requst->SetMethodType(trpc::http::MethodType::PUT);
  h = routes_->GetHandler("/test", http_requst);
  ASSERT_EQ(nullptr, h);

  http_requst->SetMethodType(trpc::http::MethodType::GET);
  h = routes_->GetHandler("/test", http_requst);
  ASSERT_EQ(h1.get(), h);

  http_requst->SetMethodType(trpc::http::MethodType::GET);
  h = routes_->GetHandler("/api/hello", http_requst);
  ASSERT_EQ(h2.get(), h);
  ASSERT_EQ("/hello", http_requst->GetMutableParameters()->At("path"));

  http_requst->SetMethodType(trpc::http::MethodType::GET);
  h = routes_->GetHandler("/channels/abc/clients/123", http_requst);
  ASSERT_EQ(h6.get(), h);
  ASSERT_EQ("abc", http_requst->GetMutableParameters()->Path("channel_id"));
  ASSERT_EQ("123", http_requst->GetMutableParameters()->Path("client_id"));

  http_requst->SetMethodType(trpc::http::MethodType::PATCH);
  h = routes_->GetHandler("/hello", http_requst);
  ASSERT_EQ(h7.get(), h);
}

TEST_F(HttpRoutesTest, Handle) {
  auto req1 = std::make_shared<trpc::http::Request>();
  auto req2 = std::make_shared<trpc::http::Request>();

  ServerContextPtr context;
  {
    trpc::http::Response reply;
    routes_->Handle("/test1", context, req1, reply);
    ASSERT_EQ(trpc::http::Response::StatusCode::kNotFound, reply.GetStatus());
  }

  {
    trpc::http::Response reply;
    req2->SetHeader("Connection", "Keep-Alive");
    routes_->Handle("/api/123", context, req2, reply);
    ASSERT_EQ(trpc::http::Response::StatusCode::kOk, reply.GetStatus());
  }
}

TEST_F(HttpRoutesTest, ExceptionReply) {
  std::exception_ptr eptr;
  try {
    std::string().at(2);
  } catch (...) {
    eptr = std::current_exception();
  }

  trpc::http::Response reply;
  routes_->ExceptionReply(eptr, &reply);
  ASSERT_EQ(trpc::http::Response::StatusCode::kInternalServerError, reply.GetStatus());
  ASSERT_EQ("{\"message\":\"Unknown unhandled exception\",\"code\":500}", reply.GetContent());

  try {
    throw(trpc::http::NotFoundException());
  } catch (...) {
    eptr = std::current_exception();
  }

  routes_->ExceptionReply(eptr, &reply);
  ASSERT_EQ(trpc::http::Response::StatusCode::kNotFound, reply.GetStatus());
  ASSERT_EQ("{\"message\":\"Not Found\",\"code\":404}", reply.GetContent());
}

}  // namespace trpc::testing
