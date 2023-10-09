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
#include "trpc/common/config/trpc_config.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/util/log/logging.h"

#include "examples/features/tvar/server/tvar.trpc.pb.h"

DEFINE_string(client_config, "trpc_cpp_client.yaml", "trpc cpp client client_config file");

namespace trpc::examples {

namespace {

#define TEST_RPC_FUNC(prx, func)                                 \
  ::trpc::examples::tvar::TestRequest req;                       \
  ::trpc::examples::tvar::TestReply rsp;                         \
  ::trpc::ClientContextPtr ctx = ::trpc::MakeClientContext(prx); \
  ::trpc::Status status = prx->func(ctx, req, &rsp);             \
  TRPC_ASSERT(status.OK())

}  // namespace

bool ExtractArgs(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::CommandLineFlagInfo info;
  if (GetCommandLineFlagInfo("client_config", &info) && info.is_default) {
    std::cerr << "start client with client_config, for example: " << argv[0] << " --client_config=client.yaml"
              << std::endl;
    return false;
  }

  return true;
}

int RunTest() {
  auto prx = ::trpc::GetTrpcClient()->GetProxy<::trpc::examples::tvar::TvarServiceProxy>("tvar_client");
  { TEST_RPC_FUNC(prx, Counter); }
  { TEST_RPC_FUNC(prx, Gauge); }
  { TEST_RPC_FUNC(prx, Miner); }
  { TEST_RPC_FUNC(prx, Maxer); }
  { TEST_RPC_FUNC(prx, Status); }
  { TEST_RPC_FUNC(prx, Averager); }
  { TEST_RPC_FUNC(prx, IntRecorder); }
  { TEST_RPC_FUNC(prx, LatencyRecorder); }
  return 0;
}

}  // namespace trpc::examples

int main(int argc, char* argv[]) {
  if (::trpc::examples::ExtractArgs(argc, argv) == false) exit(-1);

  if (::trpc::TrpcConfig::GetInstance()->Init(FLAGS_client_config) != 0) {
    std::cerr << "load client_config failed." << std::endl;
    exit(-1);
  }

  // If the business code is running in trpc pure client mode,
  // the business code needs to be running in the `RunInTrpcRuntime` function
  return ::trpc::RunInTrpcRuntime([]() { return ::trpc::examples::RunTest(); });
}
