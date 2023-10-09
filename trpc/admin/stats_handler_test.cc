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

#include "trpc/admin/stats_handler.h"

#include <iostream>
#include <memory>
#include <utility>

#include "gtest/gtest.h"
#include "rapidjson/document.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/server/server_context.h"

namespace trpc::testing {

class TestStatsHandler : public ::testing::Test {
 public:
  void SetUp() {
    EXPECT_EQ(TrpcConfig::GetInstance()->Init("trpc/admin/test2.yaml"), 0);
    EXPECT_TRUE(log::Init());
  }

  void TearDown() { log::Destroy(); }
};

TEST_F(TestStatsHandler, CommonTest) {
  http::HttpResponse reply;
  ServerContextPtr context;

  std::unique_ptr<AdminHandlerBase> h1 = std::make_unique<admin::StatsHandler>();
  h1->Handle("", context, std::make_shared<http::HttpRequest>(), &reply);
  std::cout << reply.GetContent() << std::endl;
  EXPECT_GE(reply.GetContent().size(), 10);

  http::HttpResponse reply2;
  std::unique_ptr<admin::WebStatsHandler> h2 = std::make_unique<admin::WebStatsHandler>() ;
  h2->Handle("", context, std::make_shared<http::HttpRequest>(), &reply2);
  EXPECT_GE(reply.GetContent().size(), 20);

  {
    std::unique_ptr<admin::VarHandler> h3 = std::make_unique<admin::VarHandler>("/cmds/var");
    rapidjson::Document doc;
    rapidjson::Value result(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
    http::HttpRequestPtr req = std::make_shared<trpc::http::HttpRequest>();
    req->SetUrl("/cmds/var");
    h3->CommandHandle(req, result, alloc);

    http::HttpResponse reply3;
    ServerContextPtr context3;
    h3->Handle("/cmds/var", context3, req, &reply3);
    EXPECT_GT(reply3.GetContent().size(), 0);
  }
}

#ifdef TRPC_BUILD_INCLUDE_RPCZ
TEST(TestRpczHandler, url_test) {
  std::unique_ptr<admin::RpczHandler> h = std::make_unique<admin::RpczHandler>("/cmds/rpcz");
  rapidjson::Document doc;
  rapidjson::Value result(rapidjson::kObjectType);
  doc.GetAllocator();
  http::HttpRequestPtr req = std::make_shared<trpc::http::HttpRequest>();

  req->SetUrl("cmds/");
  http::HttpResponse reply1;
  ServerContextPtr context1;
  auto status1 = h->Handle("/", context1, req, &reply1);
  EXPECT_FALSE(status1.OK());

  req->SetUrl("/cmds/rpcz");
  http::HttpResponse reply2;
  ServerContextPtr context2;
  auto status2 = h->Handle("/", context2, req, &reply2);
  EXPECT_TRUE(status2.OK());
}
#endif

}  // namespace trpc::testing
