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

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "trpc/client/make_client_context.h"
#include "trpc/client/rpc_service_proxy.h"
#include "trpc/client/trpc_client.h"
#include "trpc/common/runtime_manager.h"

DEFINE_string(target, "trpc.test.helloworld.demo_service", "callee service name");
DEFINE_string(addr, "127.0.0.1:12349", "ip:port");
DEFINE_string(client_config, "trpc_cpp_fiber.yaml", "");

int Run() {
  ::trpc::ServiceProxyOption option;

  option.name = FLAGS_target;
  option.codec_name = "trpc";
  option.network = "tcp";
  option.conn_type = "long";
  option.timeout = 1000;
  option.selector_name = "direct";
  option.target = FLAGS_addr;

  auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::RpcServiceProxy>(FLAGS_target, option);

  std::string req_json_str = "{\"age\":18,\"height\":180}";

  rapidjson::Document hello_req;
  hello_req.Parse(req_json_str.c_str());

  if (hello_req.HasParseError()) {
    std::cout << "json parse error: " << hello_req.GetParseError()
              << ", msg: " << rapidjson::GetParseError_En(hello_req.GetParseError()) << std::endl;
    return -1;
  }

  rapidjson::Document hello_rsp;

  ::trpc::ClientContextPtr context = ::trpc::MakeClientContext(proxy);
  context->SetTimeout(1000);
  context->SetFuncName("JsonSayHello");

  ::trpc::Status status = proxy->UnaryInvoke<rapidjson::Document, rapidjson::Document>(context, hello_req, &hello_rsp);

  if (!status.OK()) {
    std::cout << "invoke error: " << status.ErrorMessage() << std::endl;
    return -1;
  }

  for (rapidjson::Value::ConstMemberIterator iter = hello_rsp.MemberBegin(); iter != hello_rsp.MemberEnd(); ++iter) {
    std::cout << "json name: " << iter->name.GetString() << ", value: " << iter->value.GetInt() << std::endl;
  }

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
