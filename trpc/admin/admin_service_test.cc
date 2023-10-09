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

#include "trpc/admin/admin_service.h"

#include <iostream>
#include <memory>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/admin/commands_handler.h"
#include "trpc/admin/log_level_handler.h"
#include "trpc/admin/reload_config_handler.h"
#include "trpc/admin/version_handler.h"
#include "trpc/admin/watch_handler.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/util/log/logging.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"
#include "trpc/server/server_context.h"
#include "trpc/server/trpc_server.h"

namespace trpc {

namespace testing {

class MockHandler : public http::HandlerBase {
 public:
  ~MockHandler() override {}
  Status Handle(const std::string& path, ServerContextPtr context, http::HttpRequestPtr req,
                http::HttpResponse* rsp) override {
    return Status();
  }
};
class TestAdminService : public ::testing::Test {
 public:
  void SetUp() {
    EXPECT_EQ(TrpcConfig::GetInstance()->Init("trpc/admin/test.yaml"), 0);
    EXPECT_TRUE(log::Init());
  }

  void TearDown() { log::Destroy(); }
};

TEST_F(TestAdminService, CheckAdminService) {
  PeripheryTaskScheduler::GetInstance()->Init();
  PeripheryTaskScheduler::GetInstance()->Start();

  ServiceAdapterOption option;
  EXPECT_EQ(false, AdminService::BuildServiceAdapterOption(option));
  option.service_name = "trpc.test.test.AdminService";
  option.ip = "0.0.0.0";
  option.port = 12456;

  AdminServicePtr admin_service = std::make_shared<AdminService>();
  auto mock_handler = std::make_shared<MockHandler>();
  admin_service->RegisterCmd(http::OperationType::GET, "/cmds/mock", mock_handler);
  http::HttpRoutes& r = admin_service->GetRoutes();

  ServerContextPtr context;
  auto req1 = std::make_shared<http::HttpRequest>();
  req1->SetMethodType(http::GET);
  http::HttpResponse reply1;
  r.Handle("/cmds", context, req1, reply1);
  std::cout << reply1.GetContent() << std::endl;
  rapidjson::Document rsp1_json;
  rsp1_json.Parse(reply1.GetContent());
  ASSERT_FALSE(rsp1_json.HasParseError());
  const rapidjson::Value& cmds = rsp1_json["cmds"];
  ASSERT_TRUE(cmds.IsArray());
  ASSERT_GT(cmds.Size(), 0);

  Status status = r.Handle("/cmds/mock", context, req1, reply1);
  EXPECT_EQ(status.OK(), true);

  auto req2 = std::make_shared<http::HttpRequest>();
  req2->SetMethodType(http::GET);
  http::HttpResponse reply2;
  r.Handle("/version", context, req2, reply2);
  std::cout << reply2.GetContent() << std::endl;
  ASSERT_TRUE(!reply2.GetContent().empty());

  auto req3 = std::make_shared<http::HttpRequest>();
  req3->SetMethodType(http::GET);
  http::HttpResponse reply3;
  r.Handle("/cmds/loglevel", context, req3, reply3);
  std::cout << reply3.GetContent() << std::endl;
  rapidjson::Document rsp3_json;
  rsp3_json.Parse(reply3.GetContent());
  ASSERT_FALSE(rsp3_json.HasParseError());
  ASSERT_STREQ(rsp3_json["level"].GetString(), "INFO");

  auto req4 = std::make_shared<http::HttpRequest>();
  req4->SetMethodType(http::PUT);
  req4->SetContent("value=ERROR");
  http::HttpResponse reply4;
  r.Handle("/cmds/loglevel", context, req4, reply4);
  std::cout << reply4.GetContent() << std::endl;
  rapidjson::Document rsp4_json;
  rsp4_json.Parse(reply4.GetContent());
  ASSERT_FALSE(rsp4_json.HasParseError());
  ASSERT_STREQ(rsp4_json["level"].GetString(), "ERROR");

  auto req5 = std::make_shared<http::HttpRequest>();
  req5->SetMethodType(http::GET);
  http::HttpResponse reply5;
  r.Handle("/cmds/loglevel", context, req5, reply5);
  std::cout << reply5.GetContent() << std::endl;
  EXPECT_EQ("{\"errorcode\":0,\"message\":\"\",\"level\":\"ERROR\"}", reply5.GetContent());

  auto req6 = std::make_shared<http::HttpRequest>();
  req6->SetMethodType(http::POST);
  req6->SetContent("{}");
  http::HttpResponse reply6;
  r.Handle("/cmds/reload-config", context, req6, reply6);
  std::cout << reply6.GetContent() << std::endl;
  rapidjson::Document rsp6_json;
  rsp6_json.Parse(reply6.GetContent());
  ASSERT_FALSE(rsp6_json.HasParseError());
  ASSERT_STREQ(rsp6_json["message"].GetString(), "reload config ok");

  auto req7 = std::make_shared<http::HttpRequest>();
  req7->SetMethodType(http::POST);
  req7->SetContent("{}");
  http::HttpResponse reply7;
  r.Handle("/cmds/watch", context, req7, reply7);
  std::cout << reply7.GetContent() << std::endl;
  rapidjson::Document rsp7_json;
  rsp7_json.Parse(reply7.GetContent());
  ASSERT_FALSE(rsp7_json.HasParseError());
  ASSERT_STREQ(rsp7_json["message"].GetString(), "watching unsupported");

  auto req8 = std::make_shared<http::HttpRequest>();
  req8->SetMethodType(http::GET);
  req8->SetContent("{}");
  http::HttpResponse reply8;
  r.Handle("", context, req8, reply8);
  EXPECT_GE(reply8.GetContent().size(), 10);

  auto req9 = std::make_shared<http::HttpRequest>();
  req9->SetMethodType(http::GET);
  req9->SetContent("{}");
  http::HttpResponse reply9;
  r.Handle("/", context, req9, reply9);
  EXPECT_GE(reply9.GetContent().size(), 10);

  auto req10 = std::make_shared<http::HttpRequest>();
  req10->SetMethodType(http::GET);
  req10->SetContent("{}");
  http::HttpResponse reply10;
  r.Handle("/cmdsweb/logs", context, req10, reply10);
  EXPECT_GE(reply10.GetContent().size(), 10);

  PeripheryTaskScheduler::GetInstance()->Stop();
  PeripheryTaskScheduler::GetInstance()->Join();
}

}  // namespace testing

}  // namespace trpc
