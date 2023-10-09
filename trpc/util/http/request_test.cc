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

#include "trpc/util/http/request.h"

#include "gtest/gtest.h"

namespace trpc::testing {

class RequestTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() { req_ = new trpc::http::HttpRequest(); }

  static void TearDownTestCase() {
    delete req_;
    req_ = nullptr;
  }

  static trpc::http::Request* req_;
};

trpc::http::Request* RequestTest::req_ = nullptr;

TEST_F(RequestTest, GetHeader) {
  req_->SetHeader("Content-Type", "json");
  req_->SetHeaderIfNotPresent("Content-Type", "should-not-overwrite");

  ASSERT_EQ("json", req_->GetHeader("Content-Type"));
  ASSERT_EQ("", req_->GetHeader("trpc"));
}

TEST_F(RequestTest, GetHeader2) {
  req_->SetHeaderIfNotPresent("Content-Type", "should-overwrite");
  req_->SetHeader("Content-Type", "json");

  ASSERT_EQ("json", req_->GetHeader("Content-Type"));
  ASSERT_EQ("", req_->GetHeader("trpc"));
}

TEST_F(RequestTest, HasHeader) {
  req_->SetHeader("Content-Type", "should-overwrite");

  ASSERT_TRUE(req_->HasHeader("Content-Type"));
  ASSERT_FALSE(req_->HasHeader("Non-Existent-Header"));

  req_->DeleteHeader("Content-Type");

  ASSERT_FALSE(req_->HasHeader("Content-Type"));
  ASSERT_FALSE(req_->HasHeader("Non-Existent-Header"));
}

TEST_F(RequestTest, GetQueryParam) {
  req_->SetQueryParameter("protocol", "http");

  ASSERT_EQ("http", req_->GetQueryParameter("protocol"));
  ASSERT_EQ("", req_->GetQueryParameter("server"));
  ASSERT_EQ("default-server", req_->GetQueryParameter("server", "default-server"));
}

TEST_F(RequestTest, GetHttpPrefix) { ASSERT_EQ("HTTP/", std::string{http::kHttpPrefix}); }

TEST_F(RequestTest, GetHttpPrefixSize) { ASSERT_EQ(5, http::kHttpPrefixSize); }

TEST_F(RequestTest, GetUrl) {
  req_->SetUrl("http://127.0.0.1/trpc/hello?name=test");
  req_->SetHeader("Host", "127.0.0.1");

  ASSERT_EQ("http://127.0.0.1/trpc/hello?name=test", req_->GetUrl());
}

TEST_F(RequestTest, SetQueryParam) {
  req_->GetMutableQueryParameters()->Clear();

  req_->SetUrl("/trpc/hello");
  req_->InitQueryParameters();
  ASSERT_EQ(0, req_->GetQueryParameters().FlatPairsCount());

  req_->SetUrl("/trpc/hello?name=world&message=trpc");
  req_->InitQueryParameters();

  ASSERT_EQ("world", req_->GetQueryParameter("name"));
  ASSERT_EQ("trpc", req_->GetQueryParameter("message"));
}

TEST_F(RequestTest, GetRouteUrl) { ASSERT_EQ("/trpc/hello", req_->GetRouteUrl()); }

TEST_F(RequestTest, AddParam) {
  req_->AddQueryParameter("group");
  ASSERT_EQ("", req_->GetQueryParameter("group"));

  req_->AddQueryParameter("group=trpc");
  ASSERT_EQ("trpc", req_->GetQueryParameter("group"));
}


TEST_F(RequestTest, SerializeToString) {
  req_->SetMethodType(http::POST);
  req_->SetUrl("/testaaa");
  req_->SetVersion("1.1");

  req_->SetHeader("Content-Type", "application/json");
  req_->SetHeader("Content-Length", "3");
  req_->SetContent("abc");
  ASSERT_EQ(
      "POST /testaaa HTTP/1.1\r\n"
      "Host: 127.0.0.1\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n\r\n"
      "abc",
      req_->SerializeToString());

  NoncontiguousBufferBuilder buffer_builder;
  req_->SerializeToString(buffer_builder);
  ASSERT_EQ(
      "POST /testaaa HTTP/1.1\r\n"
      "Host: 127.0.0.1\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n\r\n"
      "abc",
      FlattenSlow(buffer_builder.DestructiveGet()));
}

TEST_F(RequestTest, SerializeToOStream) {
  req_->SetMethodType(http::POST);
  req_->SetUrl("/testaaa");
  req_->SetVersion("1.1");

  req_->SetHeader("Content-Type", "application/json");
  req_->SetHeader("Content-Length", "3");

  req_->SetContent("abc");
  std::stringstream ss_data;
  ss_data << *req_;

  ASSERT_EQ(
      "POST /testaaa HTTP/1.1\r\n"
      "Host: 127.0.0.1\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc",
      ss_data.str());
}

}  // namespace trpc::testing
