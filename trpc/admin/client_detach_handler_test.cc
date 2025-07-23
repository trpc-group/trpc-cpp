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

#include "trpc/admin/client_detach_handler.h"

#include <memory>

#include "gtest/gtest.h"

#include "trpc/server/server_context.h"

namespace trpc::testing {

constexpr char kClientDetachDescription[] = "[POST /client_detach] client detach from target";

TEST(ClientDetachHandlerTest, CommandHandle) {
  std::unique_ptr<admin::ClientDetachHandler> prom_handler = std::make_unique<admin::ClientDetachHandler>();
  ASSERT_EQ(kClientDetachDescription, prom_handler->Description());

  http::HttpRequestPtr req = std::make_shared<http::HttpRequest>();
  http::HttpResponse reply;
  trpc::Status status = prom_handler->Handle("", nullptr, req, &reply);
  ASSERT_EQ(true, status.OK());
}

}  // namespace trpc::testing
