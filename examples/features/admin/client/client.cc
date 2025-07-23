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

#include "gflags/gflags.h"

#include "examples/features/admin/proxy/forward.trpc.pb.h"
#include "trpc/client/make_client_context.h"
#include "trpc/client/trpc_client.h"
#include "trpc/common/logging/trpc_logging.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/common/status.h"

DEFINE_string(target, "trpc.test.route.Forward", "callee service name");
DEFINE_string(addr, "127.0.0.1:12346", "ip:port");
DEFINE_string(client_config, "fiber_client.yaml", "");
DEFINE_string(msg, "test", "the msg to send");

void DoRoute(const std::shared_ptr<::trpc::test::route::ForwardServiceProxy>& prx) {
  ::trpc::test::helloworld::HelloRequest request;
  request.set_msg(FLAGS_msg);

  auto context = ::trpc::MakeClientContext(prx);
  context->SetTimeout(1000);

  ::trpc::test::helloworld::HelloReply reply;
  ::trpc::Status status = prx->Route(context, request, &reply);

  std::cout << "status:" << status.ToString() << " " << reply.msg() << std::endl;
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

  auto prx = ::trpc::GetTrpcClient()->GetProxy<::trpc::test::route::ForwardServiceProxy>(FLAGS_target, option);

  DoRoute(prx);

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
