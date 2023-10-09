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

#include "trpc/admin/sysvars_handler.h"

#include <string.h>

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/server/server_context.h"

namespace trpc::testing {

TEST(WebSysVarsHandlerTest, Test) {
  http::HttpResponse reply;
  trpc::ServerContextPtr context;

  std::unique_ptr<AdminHandlerBase> h1 = std::make_unique<admin::WebSysVarsHandler>();
  h1->Handle("", context, std::make_shared<http::HttpRequest>(), &reply);
  EXPECT_GE(reply.GetContent().size(), 10);

  std::string html;
  admin::WebSysVarsHandler::PrintSysVarsData(&html);
  EXPECT_GE(html.size(), 10);

  html.clear();
  admin::WebSysVarsHandler::PrintSysVarsData(&html, true);
  EXPECT_GE(html.size(), 10);

  auto req2 = std::make_shared<http::HttpRequest>();
  req2->SetUrl("http://127.0.0.1:8080/seriesvar?var=proc_loadavg_1m");
  http::HttpResponse reply2;
  std::unique_ptr<AdminHandlerBase> h2 = std::make_unique<admin::WebSeriesVarHandler>();
  h2->Handle("", context, req2, &reply2);
  EXPECT_EQ(reply2.GetStatus(), trpc::http::HttpResponse::StatusCode::kOk);
}

}  // namespace trpc::testing
