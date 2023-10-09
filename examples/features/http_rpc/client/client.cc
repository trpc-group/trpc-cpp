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
#include <vector>

#include "gflags/gflags.h"

#include "trpc/client/http/http_service_proxy.h"
#include "trpc/client/make_client_context.h"
#include "trpc/client/trpc_client.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/common/status.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/util/log/logging.h"

#include "examples/helloworld/helloworld.trpc.pb.h"

DEFINE_string(service_name, "http_client", "callee service name");
DEFINE_string(client_config, "trpc_cpp_fiber.yaml", "");
DEFINE_string(addr, "127.0.0.1:24857", "ip:port");

namespace http::demo {

constexpr std::string_view kGreetings{"hello world!"};

using HttpServiceProxyPtr = std::shared_ptr<::trpc::http::HttpServiceProxy>;

namespace {
//// Unary invoke.

bool UnaryInvoke(const HttpServiceProxyPtr& proxy) {
  auto ctx = ::trpc::MakeClientContext(proxy);
  ctx->SetTimeout(5000);
  ctx->SetFuncName("/trpc.test.helloworld.Greeter/SayHello");
  ::trpc::test::helloworld::HelloRequest req;
  ::trpc::test::helloworld::HelloReply rsp;

  req.set_msg("hello world!");
  auto status =
      proxy->UnaryInvoke<::trpc::test::helloworld::HelloRequest, ::trpc::test::helloworld::HelloReply>(ctx, req, &rsp);
  if (!status.OK()) {
    TRPC_FMT_ERROR("status: {}", status.ToString());
    return false;
  }
  TRPC_FMT_INFO("response content: {}", rsp.msg());
  if (rsp.msg().find(req.msg()) == std::string::npos) return false;
  return true;
}
}  // namespace

int Run() {
  bool final_ok{true};

  struct http_calling_args_t {
    std::string calling_name;
    std::function<bool()> calling_executor;
    bool ok;
  };

  ::trpc::ServiceProxyOption option;
  option.name = FLAGS_service_name;
  option.codec_name = "trpc_over_http";
  option.network = "tcp";
  option.conn_type = "long";
  option.timeout = 1000;
  option.selector_name = "direct";
  option.target = FLAGS_addr;

  auto http_client = ::trpc::GetTrpcClient()->GetProxy<::trpc::http::HttpServiceProxy>(FLAGS_service_name, option);
  std::vector<http_calling_args_t> callings{
      // Unary invoke.
      // Executing multiple cases is to send concurrent requests.
      {"Unary invoking-1", [&http_client]() { return UnaryInvoke(http_client); }, false},
      {"Unary invoking-2", [&http_client]() { return UnaryInvoke(http_client); }, false},
      {"Unary invoking-3", [&http_client]() { return UnaryInvoke(http_client); }, false},
  };

  auto latch_count = static_cast<std::ptrdiff_t>(callings.size());
  ::trpc::FiberLatch callings_latch{latch_count};

  for (auto& c : callings) {
    ::trpc::StartFiberDetached([&callings_latch, &c]() {
      c.ok = c.calling_executor();
      callings_latch.CountDown();
    });
  }

  callings_latch.Wait();

  for (const auto& c : callings) {
    final_ok &= c.ok;
    std::cout << "name: " << c.calling_name << ", ok: " << c.ok << std::endl;
  }

  std::cout << "final result of http calling: " << final_ok << std::endl;
  return final_ok ? 0 : -1;
}

}  // namespace http::demo

void ParseClientConfig(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::CommandLineFlagInfo info;
  if (GetCommandLineFlagInfo("client_config", &info) && info.is_default) {
    std::cerr << "start client with client_config, for example: " << argv[0]
              << " --client_config=/client/client_config/filepath" << std::endl;
    exit(-1);
  }
  std::cout << "FLAGS_service_name: " << FLAGS_service_name << std::endl;
  std::cout << "FLAGS_client_config: " << FLAGS_client_config << std::endl;
  std::cout << "FLAGS_addr: " << FLAGS_addr << std::endl;
}

int main(int argc, char* argv[]) {
  ParseClientConfig(argc, argv);

  if (::trpc::TrpcConfig::GetInstance()->Init(FLAGS_client_config) != 0) {
    std::cerr << "load client_config failed." << std::endl;
    exit(-1);
  }

  // If the business code is running in trpc pure client mode,
  // the business code needs to be running in the `RunInTrpcRuntime` function
  return ::trpc::RunInTrpcRuntime([]() { return http::demo::Run(); });
}
