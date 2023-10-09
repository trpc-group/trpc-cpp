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

#include <iostream>
#include <string>

#include "gflags/gflags.h"

#include "trpc/client/make_client_context.h"
#include "trpc/client/trpc_client.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/future/future_utility.h"
#include "trpc/log/trpc_log.h"

#include "examples/helloworld/helloworld.trpc.pb.h"

DEFINE_string(client_config, "trpc_cpp.yaml", "framework client_config file, --client_config=trpc_cpp.yaml");
DEFINE_string(service_name, "trpc.test.helloworld.Greeter", "callee service name");

int DoAsyncRpcCall(const std::shared_ptr<::trpc::test::helloworld::GreeterServiceProxy>& proxy) {
  ::trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(proxy);
  ::trpc::test::helloworld::HelloRequest req;
  req.set_msg("future");
  bool succ = true;
  auto fut = proxy->AsyncSayHello(client_ctx, req)
      .Then([&succ](::trpc::Future<::trpc::test::helloworld::HelloReply>&& fut) {
        if (fut.IsReady()) {
          auto rsp = fut.GetValue0();
          std::cout << "get rsp msg: " << rsp.msg() << std::endl;
        } else {
          auto exception = fut.GetException();
          succ = false;
          std::cerr << "get rpc error: " << exception.what() << std::endl;
        }
        return ::trpc::MakeReadyFuture<>();
      });
  ::trpc::future::BlockingGet(std::move(fut));
  return succ ? 0 : -1;
}

int Run() {
  auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::test::helloworld::GreeterServiceProxy>(FLAGS_service_name);

  return DoAsyncRpcCall(proxy);
}

void ParseClientConfig(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::CommandLineFlagInfo info;
  if (GetCommandLineFlagInfo("client_config", &info) && info.is_default) {
    std::cerr << "start client with client_config, for example: " << argv[0]
              << " --client_config=/client/client_config/filepath" << std::endl;
    exit(-1);
  }

  std::cout << "FLAGS_service_name:" << FLAGS_service_name << std::endl;
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
  // This function can be seen as a program entry point and should be called only once.
  return ::trpc::RunInTrpcRuntime([]() { return Run(); });
}
