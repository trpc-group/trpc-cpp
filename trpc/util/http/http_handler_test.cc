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

#include "trpc/util/http/http_handler.h"

#include "gtest/gtest.h"

#include "trpc/server/server_context.h"

namespace trpc::testing {


class TestMethodDispatchHttpHandler : public http::HttpHandler {
 public:
  http::MethodType last_method;

  trpc::Status Get(const ServerContextPtr&, const http::RequestPtr&, http::Response* rsp) override {
    last_method = http::GET;
    return trpc::kSuccStatus;
  }

  trpc::Status Head(const ServerContextPtr&, const http::RequestPtr&, http::Response* rsp) override {
    last_method = http::HEAD;
    return trpc::kSuccStatus;
  }

  trpc::Status Post(const ServerContextPtr&, const http::RequestPtr&, http::Response* rsp) override {
    last_method = http::POST;
    return trpc::kSuccStatus;
  }

  trpc::Status Put(const ServerContextPtr&, const http::RequestPtr&, http::Response* rsp) override {
    last_method = http::PUT;
    return trpc::kSuccStatus;
  }

  trpc::Status Delete(const ServerContextPtr&, const http::RequestPtr&, http::Response* rsp) override {
    last_method = http::DELETE;
    return trpc::kSuccStatus;
  }

  trpc::Status Options(const ServerContextPtr&, const http::RequestPtr&, http::Response* rsp) override {
    last_method = http::OPTIONS;
    return trpc::kSuccStatus;
  }

  trpc::Status Patch(const ServerContextPtr&, const http::RequestPtr&, http::Response* rsp) override {
    last_method = http::PATCH;
    return trpc::kSuccStatus;
  }
};

class TestExceptionHttpHandler : public http::HttpHandler {
 public:
  trpc::Status Get(const ServerContextPtr&, const http::RequestPtr&, http::Response* rsp) override {
    throw http::BaseException("Test", http::Response::StatusCode::kBadRequest);
    return trpc::kSuccStatus;
  }

  trpc::Status Post(const ServerContextPtr&, const http::RequestPtr&, http::Response* rsp) override {
    throw std::runtime_error("Test2");
    return trpc::kSuccStatus;
  }
};

class HttpHandlerTest : public ::testing::Test {};

TEST_F(HttpHandlerTest, TestMethodDispatch) {
  TestMethodDispatchHttpHandler handler;
  ServerContextPtr ctx;
  auto req = std::make_shared<trpc::http::Request>();
  trpc::http::Response rsp;

  req->SetMethodType(http::GET);
  handler.Handle("", ctx, req, &rsp);
  EXPECT_EQ(http::GET, handler.last_method);
  req->SetMethodType(http::HEAD);
  handler.Handle("", ctx, req, &rsp);
  EXPECT_EQ(http::HEAD, handler.last_method);
  req->SetMethodType(http::POST);
  handler.Handle("", ctx, req, &rsp);
  EXPECT_EQ(http::POST, handler.last_method);
  req->SetMethodType(http::PUT);
  handler.Handle("", ctx, req, &rsp);
  EXPECT_EQ(http::PUT, handler.last_method);
  req->SetMethodType(http::DELETE);
  handler.Handle("", ctx, req, &rsp);
  EXPECT_EQ(http::DELETE, handler.last_method);
  req->SetMethodType(http::OPTIONS);
  handler.Handle("", ctx, req, &rsp);
  EXPECT_EQ(http::OPTIONS, handler.last_method);
  req->SetMethodType(http::PATCH);
  handler.Handle("", ctx, req, &rsp);
  EXPECT_EQ(http::PATCH, handler.last_method);
}

TEST_F(HttpHandlerTest, TestMethodException) {
  TestExceptionHttpHandler handler;
  ServerContextPtr ctx;
  auto req = std::make_shared<trpc::http::Request>();
  trpc::http::Response rsp1, rsp2, rsp3;

  req->SetMethodType(http::GET);
  handler.Handle("", ctx, req, &rsp1);
  EXPECT_EQ("Test", rsp1.GetContent());
  EXPECT_EQ(http::Response::StatusCode::kBadRequest, rsp1.GetStatus());

  req->SetMethodType(http::POST);
  handler.Handle("", ctx, req, &rsp2);
  EXPECT_EQ("", rsp2.GetContent());  // should not return internal error to users
  EXPECT_EQ(http::Response::StatusCode::kInternalServerError, rsp2.GetStatus());

  req->SetMethodType(http::PATCH);
  handler.Handle("", ctx, req, &rsp3);
  EXPECT_EQ("", rsp3.GetContent());
  EXPECT_EQ(http::Response::StatusCode::kNotImplemented, rsp3.GetStatus());
}

}  // namespace trpc::testing
