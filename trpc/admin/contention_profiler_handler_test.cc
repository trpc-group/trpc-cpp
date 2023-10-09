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

#include "trpc/admin/contention_profiler_handler.h"

#include <string.h>

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/server/server_context.h"

namespace trpc::testing {

TEST(WebContentionProfilerHandlerTest, Test) {
  ServerContextPtr context;

  http::HttpResponse reply;
  std::unique_ptr<AdminHandlerBase> h1 = std::make_unique<admin::WebContentionProfilerHandler>();
  h1->Handle("", context, std::make_shared<http::HttpRequest>(), &reply);
  EXPECT_EQ("alert(please recomplile with -DTRPC_ENABLE_PROFILER)", reply.GetContent());

  http::HttpResponse reply2;
  std::unique_ptr<AdminHandlerBase> h2 = std::make_unique<admin::WebContentionProfilerDrawHandler>();
  h2->Handle("", context, std::make_shared<http::HttpRequest>(), &reply2);
  EXPECT_EQ("alert(please recomplile with -DTRPC_ENABLE_PROFILER)", reply2.GetContent());
}

}  // namespace trpc::testing
