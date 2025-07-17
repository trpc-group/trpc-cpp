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
#include "trpc/common/runtime_manager.h"

#include "examples/features/trpc_flatbuffers/proxy/forward.trpc.fb.h"
#include "examples/features/trpc_flatbuffers/proxy/forward_generated.h"

DEFINE_string(target, "trpc.test.forward.FbForward", "callee service name");
DEFINE_string(addr, "127.0.0.1:12349", "ip:port");
DEFINE_string(client_config, "trpc_cpp_fiber.yaml", "");

void DoForward(std::shared_ptr<::trpc::test::forward::FbForwardServiceProxy>& proxy) {
  std::string msg("hello world");

  ::flatbuffers::trpc::MessageBuilder mb;
  auto name_offset = mb.CreateString(msg);
  auto request_offset = ::trpc::test::helloworld::CreateFbRequest(mb, name_offset);
  mb.Finish(request_offset);
  auto request_msg = mb.ReleaseMessage<::trpc::test::helloworld::FbRequest>();

  const ::trpc::test::helloworld::FbRequest* request = request_msg.GetRoot();

  std::cout << "request msg: " << request->message()->str() << std::endl;
  ;

  ::flatbuffers::trpc::Message<::trpc::test::helloworld::FbReply> response_msg;

  auto context = trpc::MakeClientContext(proxy);
  context->SetTimeout(1000);

  auto status = proxy->Forward(context, request_msg, &response_msg);
  if (status.OK()) {
    const trpc::test::helloworld::FbReply* response = response_msg.GetRoot();
    std::cout << "response msg: " << response->message()->str() << std::endl;
  } else {
    std::cout << "error: " << status.ErrorMessage() << std::endl;
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

  auto prx = ::trpc::GetTrpcClient()->GetProxy<::trpc::test::forward::FbForwardServiceProxy>(FLAGS_target, option);

  DoForward(prx);

  return 0;
}

void ParseClientConfig(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::CommandLineFlagInfo info;
  if (GetCommandLineFlagInfo("client_config", &info) && info.is_default) {
    TRPC_FMT_ERROR("start client with client_config, for example:{} --client_config=/client/client_config/filepath",
                   argv[0]);
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
