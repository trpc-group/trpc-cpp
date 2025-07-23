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


#include "trpc/admin/version_handler.h"

#include <memory>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/server/server_context.h"

namespace trpc::testing {

constexpr char kVersionDescription[] = "[GET /version]    get trpc version";

TEST(TestVersionHandler, Test) {
  std::unique_ptr<AdminHandlerBase> h1 = std::make_unique<admin::VersionHandler>();
  EXPECT_EQ(kVersionDescription, h1->Description());

  http::HttpRequestPtr req = std::make_shared<http::HttpRequest>();
  http::HttpResponse reply;
  ServerContextPtr context;
  h1->Handle("", context, req, &reply);
  EXPECT_GE(reply.GetContent().size(), 0);
}

}  // namespace trpc::testing
