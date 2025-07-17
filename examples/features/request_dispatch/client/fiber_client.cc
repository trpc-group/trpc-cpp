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

#include <atomic>
#include <csignal>
#include <cstdint>

#include "gflags/gflags.h"

#include "trpc/client/make_client_context.h"
#include "trpc/client/trpc_client.h"
#include "trpc/common/logging/trpc_logging.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/util/latch.h"
#include "trpc/util/time.h"

#include "examples/helloworld/helloworld.trpc.pb.h"

DEFINE_string(target, "trpc.test.helloworld.Greeter", "callee service name");
DEFINE_string(addr, "127.0.0.1:12353", "ip:port");
DEFINE_string(client_config, "trpc_cpp_fiber.yaml", "");
DEFINE_uint32(count, 32, "");

std::atomic<uint64_t> g_succ = 0;
std::atomic<uint64_t> g_fail = 0;

void DoRoute(const std::shared_ptr<::trpc::test::helloworld::GreeterServiceProxy>& prx, uint32_t index) {
  ::trpc::test::helloworld::HelloRequest request;
  request.set_msg(std::to_string(index));

  auto context = ::trpc::MakeClientContext(prx);
  context->SetTimeout(1000);
  context->AddReqTransInfo("uin", std::to_string(index));

  ::trpc::test::helloworld::HelloReply reply;
  ::trpc::Status status = prx->SayHello(context, request, &reply);

  if (status.OK()) {
    ++g_succ;
  } else {
    ++g_fail;
    std::cout << "error msg:" << status.ToString() << std::endl;
  }
}

int Run() {
  ::trpc::ServiceProxyOption option;

  option.name = FLAGS_target;
  option.codec_name = "trpc";
  option.network = "tcp";
  option.conn_type = "long";
  option.timeout = 1000;
  option.selector_name = "direct";
  option.target = FLAGS_addr;

  auto prx = ::trpc::GetTrpcClient()->GetProxy<::trpc::test::helloworld::GreeterServiceProxy>(FLAGS_target, option);

  size_t begin_time = ::trpc::time::GetMilliSeconds();

  uint32_t k = 0;
  while (k < FLAGS_count) {
    DoRoute(prx, k);

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

  std::cout << "FLAGS_target:" << FLAGS_target << std::endl;
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
