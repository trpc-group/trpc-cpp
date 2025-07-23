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

#include <iostream>
#include <string>

#include "gflags/gflags.h"

#include "trpc/client/make_client_context.h"
#include "trpc/client/trpc_client.h"
#include "trpc/common/logging/trpc_logging.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/future/future_utility.h"

#include "examples/features/filter/common/invoke_stat_filter.h"
#include "examples/features/filter/common/user_rpc_filter.h"
#include "examples/helloworld/helloworld.trpc.pb.h"

using GreeterProxyPtr = std::shared_ptr<::trpc::test::helloworld::GreeterServiceProxy>;

DEFINE_string(client_config, "trpc_cpp_separate.yaml", "trpc cpp framework client_config file");

void ParseClientConfig(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, false);
  google::CommandLineFlagInfo info;
  if (GetCommandLineFlagInfo("client_config", &info) && info.is_default) {
    TRPC_FMT_ERROR("start client with client_config, for example:{} --client_config=/client/client_config/filepath",
                   argv[0]);
    exit(-1);
  }

  int ret = ::trpc::TrpcConfig::GetInstance()->Init(FLAGS_client_config);
  if (ret != 0) {
    std::cerr << "load client_config failed." << std::endl;
    exit(-1);
  }
}

void DoPbAsyncSayHello(GreeterProxyPtr proxy_ptr) {
  ::trpc::ClientContextPtr client_context_ptr = ::trpc::MakeClientContext(proxy_ptr);
  client_context_ptr->SetTimeout(1000);

  ::trpc::test::helloworld::HelloRequest request;
  request.set_msg("async_future");
  auto fut = proxy_ptr->AsyncSayHello(client_context_ptr, request)
                 .Then([](::trpc::Future<::trpc::test::helloworld::HelloReply>&& rsp_fut) {
                   if (rsp_fut.IsFailed()) {
                     auto ex = rsp_fut.GetException();
                     std::cout << ex.what() << std::endl;
                     return ::trpc::MakeExceptionFuture<>(ex);
                   }

                   auto response = rsp_fut.GetValue0();
                   std::cout << response.msg() << std::endl;
                   return ::trpc::MakeReadyFuture<>();
                 });
  ::trpc::future::BlockingGet(std::move(fut));
}

void DoPbSayHello(GreeterProxyPtr proxy_ptr) {
  ::trpc::test::helloworld::HelloRequest request;
  ::trpc::test::helloworld::HelloReply response;
  request.set_msg("sync_future");

  ::trpc::ClientContextPtr client_context_ptr = ::trpc::MakeClientContext(proxy_ptr);
  client_context_ptr->SetTimeout(1000);

  ::trpc::Status status = proxy_ptr->SayHello(client_context_ptr, request, &response);
  if (status.OK()) {
    std::cout << response.msg() << std::endl;
  } else {
    std::cout << status.ErrorMessage() << std::endl;
  }
}

int Run() {
  std::shared_ptr<::trpc::TrpcClient> client_ptr = ::trpc::GetTrpcClient();
  GreeterProxyPtr proxy_ptr =
      client_ptr->GetProxy<::trpc::test::helloworld::GreeterServiceProxy>("trpc.test.helloworld.Greeter");

  DoPbAsyncSayHello(proxy_ptr);

  DoPbSayHello(proxy_ptr);

  return 0;
}

int main(int argc, char* argv[]) {
  ParseClientConfig(argc, argv);

  // Register filters you need.
  // 1. Register filter base on RpcClientFilter
  ::trpc::MessageClientFilterPtr pb_rpc_filter = std::make_shared<examples::filter::UserGreeterPbRpcClientFilter>();
  ::trpc::TrpcPlugin::GetInstance()->RegisterClientFilter(pb_rpc_filter);
  // 2. Register filter base on MessageClientFilter
  ::trpc::MessageClientFilterPtr invoke_stat_filter = std::make_shared<examples::filter::ClientInvokeStatFilter>();
  ::trpc::TrpcPlugin::GetInstance()->RegisterClientFilter(invoke_stat_filter);

  // If the business code is running in trpc pure client mode,
  // the business code needs to be running in the `RunInTrpcRuntime` function
  return ::trpc::RunInTrpcRuntime([]() { return Run(); });
}
