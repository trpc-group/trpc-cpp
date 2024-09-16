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

#include <gflags/gflags.h>

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "trpc/client/client_context.h"
#include "trpc/client/make_client_context.h"
#include "trpc/client/mysql/mysql_service_proxy.h"
#include "trpc/client/trpc_client.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/log/trpc_log.h"
#include "trpc/util/latch.h"

DEFINE_string(client_config, "fiber_client_client_config.yaml", "trpc cpp framework client_config file");

void TestQuery(std::shared_ptr<trpc::mysql::MysqlServiceProxy>& proxy) {
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  trpc::mysql::MysqlResults<int, std::string> res;
  proxy->Query(ctx, res, "select id, username from users where id = ?", 1);
  auto& res_data = res.GetResultSet();
}

int Run() {
  auto proxy = trpc::GetTrpcClient()->GetProxy<trpc::mysql::MysqlServiceProxy>("mysql_server");
  TestQuery(proxy);
}

void ParseClientConfig(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::CommandLineFlagInfo info;
  if (GetCommandLineFlagInfo("client_config", &info) && info.is_default) {
    std::cerr << "start client with client_config, for example: " << argv[0]
              << " --client_config=/client/client_config/filepath" << std::endl;
    exit(-1);
  }
  std::cout << "FLAGS_client_config:" << FLAGS_client_config << std::endl;

  int ret = ::trpc::TrpcConfig::GetInstance()->Init(FLAGS_client_config);
  if (ret != 0) {
    std::cerr << "load client_config failed." << std::endl;
    exit(-1);
  }
}

int main(int argc, char* argv[]) {
  ParseClientConfig(argc, argv);

  // If the business code is running in trpc pure client mode,
  // the business code needs to be running in the `RunInTrpcRuntime` function
  return ::trpc::RunInTrpcRuntime([]() { return Run(); });
}