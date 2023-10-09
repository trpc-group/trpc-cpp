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

#include "trpc/admin/js_handler.h"

#include <string.h>
#include <iostream>

#include "gtest/gtest.h"

#include "trpc/server/server_context.h"

namespace trpc::testing {

constexpr char kJQueryDescription[] = "[GET /cmdsweb/js/jquery_min] get jquery_min.js";

constexpr char kVizDescription[] = "[GET /cmdsweb/js/viz_min] get viz_min.js";

constexpr char kFlotDescription[] = "[GET /cmdsweb/js/flot_min] get flot_min.js";

TEST(JsTest, Test) {
  ServerContextPtr context;

  std::unique_ptr<AdminHandlerBase> h1 = std::make_unique<admin::JqueryJsHandler>();
  EXPECT_EQ(kJQueryDescription, h1->Description());
  http::HttpRequestPtr req1 = std::make_shared<http::HttpRequest>();
  http::HttpResponse reply1;
  h1->Handle("", context, req1, &reply1);
  EXPECT_GE(reply1.GetContent().size(), 10);

  std::unique_ptr<AdminHandlerBase> h2 = std::make_unique<admin::VizJsHandler>();
  EXPECT_EQ(kVizDescription, h2->Description());
  http::HttpRequestPtr req2 = std::make_shared<http::HttpRequest>();
  http::HttpResponse reply2;
  h2->Handle("", context, req2, &reply2);
  EXPECT_GE(reply2.GetContent().size(), 10);

  std::unique_ptr<AdminHandlerBase> h3 = std::make_unique<admin::FlotJsHandler>();
  EXPECT_EQ(kFlotDescription, h3->Description());
  http::HttpRequestPtr req3 = std::make_shared<http::HttpRequest>();
  http::HttpResponse reply3;
  h3->Handle("", context, req3, &reply3);
  EXPECT_GE(reply3.GetContent().size(), 10);
}

}  // namespace trpc::testing
