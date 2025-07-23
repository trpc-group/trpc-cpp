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

#ifdef TRPC_BUILD_INCLUDE_PROMETHEUS
#include <memory>

#include "gtest/gtest.h"

#include "trpc/admin/prometheus_handler.h"
#include "trpc/server/server_context.h"

namespace trpc::testing {

TEST(PrometheusHandlerTest, CommandHandle) {
  std::unique_ptr<admin::PrometheusHandler> prom_handler = std::make_unique<admin::PrometheusHandler>();

  http::RequestPtr req = std::make_shared<http::Request>();
  http::Response resp;
  trpc::Status status = prom_handler->Handle("", nullptr, req, &resp);
  ASSERT_EQ(true, status.OK());
  ASSERT_NE(0, resp.GetContent().size());
}

}  // namespace trpc::testing
#endif
