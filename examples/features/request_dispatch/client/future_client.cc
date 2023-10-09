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
#include "trpc/runtime/threadmodel/common/worker_thread.h"
#include "trpc/util/thread/latch.h"

#include "examples/helloworld/helloworld.trpc.pb.h"

DEFINE_string(service, "trpc.test.helloworld.Greeter", "callee service name");
DEFINE_string(addr, "127.0.0.1:12353", "ip:port");
DEFINE_string(client_config, "trpc_cpp_fiber.yaml", "");
DEFINE_uint32(count, 32, "");

std::atomic<uint64_t> g_succ = 0;
std::atomic<uint64_t> g_fail = 0;
uint32_t g_iothread_num = 0;

void DoAsyncRpcCall(const std::shared_ptr<::trpc::test::helloworld::GreeterServiceProxy>& proxy, uint32_t index) {
  ::trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(proxy);
  client_ctx->SetTimeout(1000);
  client_ctx->AddReqTransInfo("uin", std::to_string(index));
  client_ctx->SetRequestDispatchKey(index);

  ::trpc::test::helloworld::HelloRequest req;
  req.set_msg(std::to_string(index));

  auto fut = proxy->AsyncSayHello(client_ctx, req)
      .Then([index](::trpc::Future<::trpc::test::helloworld::HelloReply>&& fut) {
        ::trpc::WorkerThread* current_thread = ::trpc::WorkerThread::GetCurrentWorkerThread();
        TRPC_ASSERT(current_thread != nullptr);
        TRPC_ASSERT(current_thread->Id() == (index % g_iothread_num));

        if (fut.IsReady()) {
          ++g_succ;

          // you can use the following code to get reply result
          // auto reply = fut.GetValue0();
        } else {
          ++g_fail;
          auto exception = fut.GetException();
          std::cout << "error: " << exception.what() << std::endl;
        }
        return ::trpc::MakeReadyFuture<>();
      });
  ::trpc::future::BlockingGet(std::move(fut));
  std::cout << "request  index:" << index << std::endl;
  std::cout << "begin time(ms):" << client_ctx->GetBeginTimestampUs() << std::endl;
  std::cout << "send  time(ms):" << client_ctx->GetSendTimestampUs() << std::endl;
  std::cout << "recv  time(ms):" << client_ctx->GetRecvTimestampUs() << std::endl;
  std::cout << "end   time(ms):" << client_ctx->GetEndTimestampUs() << std::endl;
}

int Run() {
  ::trpc::ServiceProxyOption option;

  option.name = FLAGS_service;
  option.codec_name = "trpc";
  option.network = "tcp";
  option.conn_type = "long";
  option.timeout = 1000;
  option.selector_name = "direct";
  option.target = FLAGS_addr;

  auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::test::helloworld::GreeterServiceProxy>(FLAGS_service, option);

  size_t begin_time = ::trpc::time::GetMilliSeconds();

  uint32_t k = 0;
  while (k < FLAGS_count) {
    DoAsyncRpcCall(proxy, k);

    ++k;
  }

  size_t end_time = ::trpc::time::GetMilliSeconds();

  std::cout << "succ:" << g_succ << ", fail:" << g_fail << ", timecost(ms):" << (end_time - begin_time) << std::endl;

  return 0;
}

void ParseClientConfig(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::CommandLineFlagInfo info;
  if (GetCommandLineFlagInfo("client_config", &info) && info.is_default) {
    std::cerr << "start client with client_config, for example: " << argv[0]
              << " --client_config=/client/client_config/filepath" << std::endl;
    exit(-1);
  }

  std::cout << "FLAGS_service:" << FLAGS_service << std::endl;
  std::cout << "FLAGS_client_config:" << FLAGS_client_config << std::endl;

  int ret = ::trpc::TrpcConfig::GetInstance()->Init(FLAGS_client_config);
  if (ret != 0) {
    std::cerr << "load client_config failed." << std::endl;
    exit(-1);
  }

  const ::trpc::GlobalConfig& global_conf = ::trpc::TrpcConfig::GetInstance()->GetGlobalConfig();
  TRPC_ASSERT(global_conf.threadmodel_config.default_model.size() == 1);
  g_iothread_num = global_conf.threadmodel_config.default_model[0].io_thread_num;
}

int main(int argc, char* argv[]) {
  ParseClientConfig(argc, argv);

  // If the business code is running in trpc pure client mode,
  // the business code needs to be running in the `RunInTrpcRuntime` function
  // This function can be seen as a program entry point and should be called only once.
  return ::trpc::RunInTrpcRuntime([]() { return Run(); });
}
