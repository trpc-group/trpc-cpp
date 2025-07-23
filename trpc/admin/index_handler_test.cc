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

#include "trpc/admin/index_handler.h"

#include <string.h>
#include <iostream>
#include <memory>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/server/server_context.h"

namespace trpc::testing {

constexpr char kIndexDescription[] = "[GET /] get admin service list";

TEST(IndexHandlerTest, Test) {
  trpc::ServerContextPtr context;

  std::unique_ptr<AdminHandlerBase> h1 = std::make_unique<admin::IndexHandler>();
  EXPECT_EQ(kIndexDescription, h1->Description());

  http::HttpRequestPtr req = std::make_shared<http::HttpRequest>();
  http::HttpResponse reply;
  h1->Handle("", context, req, &reply);
  EXPECT_GE(reply.GetContent().size(), 10);
}

}  // namespace trpc::testing
