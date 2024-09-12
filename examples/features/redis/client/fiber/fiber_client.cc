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
#include "trpc/client/redis/cmdgen.h"
#include "trpc/client/redis/redis_service_proxy.h"
#include "trpc/client/redis/reply.h"
#include "trpc/client/trpc_client.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/log/trpc_log.h"
#include "trpc/util/latch.h"

DEFINE_string(client_config, "fiber_client_client_config.yaml", "trpc cpp framework client_config file");

std::atomic<int> gProgramRet = 0;

// set cmd
void TestSet(std::shared_ptr<trpc::redis::RedisServiceProxy>& proxy) {
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  trpc::redis::Reply reply;
  auto status = proxy->Command(ctx, &reply, trpc::redis::cmdgen{}.set("trpc_cpp_redis_fiber", "support_redis"));

  if (!status.OK()) {
    // RPC get error,maybe timeout
    std::cout << "Fiber call redis set command fail, error msg:" << status.ErrorMessage() << std::endl;
    gProgramRet = -1;
  } else if (reply.IsError()) {
    // Redis-Server return error
    std::cout << "Fiber call redis set command fail, error msg:" << reply.GetString() << std::endl;
  } else {
    // Success
    std::cout << "Fiber call redis set command success." << std::endl;
  }
}

// get cmd
void TestGet(std::shared_ptr<trpc::redis::RedisServiceProxy>& proxy) {
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  trpc::redis::Reply reply;
  auto status = proxy->Command(ctx, &reply, trpc::redis::cmdgen{}.get("trpc_cpp_redis_fiber"));

  if (!status.OK()) {
    // RPC get error,maybe timeout
    std::cout << "Fiber call redis get command fail, error msg:" << status.ErrorMessage() << std::endl;
    gProgramRet = -1;
  } else if (reply.IsError()) {
    // Redis-Server return error
    std::cout << "Fiber call redis get command fail, error msg:" << reply.GetString() << std::endl;
  } else if (reply.IsNil()) {
    // Redis-Server return Nil if not exists this redis-key
    std::cout << "Fiber call redis get command return Nil" << std::endl;
  } else {
    // Success
    std::cout << "Fiber call redis get command success, reply:" << reply.GetString() << std::endl;
  }
}

// mset cmd
void TestMset(std::shared_ptr<trpc::redis::RedisServiceProxy>& proxy) {
  std::vector<std::pair<std::string, std::string>> key_values;
  key_values.push_back(std::make_pair("trpc_cpp_redis_fiber_key1", "value1"));
  key_values.push_back(std::make_pair("trpc_cpp_redis_fiber_key2", "value2"));
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  trpc::redis::Reply reply;
  auto status = proxy->Command(ctx, &reply, trpc::redis::cmdgen{}.mset(key_values));

  if (!status.OK()) {
    // RPC get error,maybe timeout
    std::cout << "Fiber call redis mset command fail, error msg:" << status.ErrorMessage() << std::endl;
    gProgramRet = -1;
  } else if (reply.IsError()) {
    // Redis-Server return error
    std::cout << "Fiber call redis mset command fail, error msg:" << reply.GetString() << std::endl;
  } else {
    // Success
    std::cout << "Fiber call redis mset command success." << std::endl;
  }
}

// mget cmd
void TestMget(std::shared_ptr<trpc::redis::RedisServiceProxy>& proxy) {
  std::vector<std::string> keys;
  keys.push_back("trpc_cpp_redis_fiber_key1");
  keys.push_back("trpc_cpp_redis_fiber_key2");
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  trpc::redis::Reply reply;

  auto status = proxy->Command(ctx, &reply, trpc::redis::cmdgen{}.mget(keys));
  if (!status.OK()) {
    // RPC get error,maybe timeout
    std::cout << "Fiber call redis mget command fail, error msg:" << status.ErrorMessage() << std::endl;
    gProgramRet = -1;
  } else if (reply.IsError()) {
    // Redis-Server return error
    std::cout << "Fiber call redis mget command fail, error msg:" << reply.GetString() << std::endl;
  } else {
    // Success 
    std::cout << "Fiber call redis mget command success, reply:" << reply.GetArray().at(0).GetString() << ","
              << reply.GetArray().at(1).GetString() << std::endl;
  }
}

int Run() {
  // get RedisServiceProxy
  auto proxy = trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("redis_server");

  TestSet(proxy);

  TestGet(proxy);

  TestMset(proxy);

  TestMget(proxy);

  return gProgramRet;
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
