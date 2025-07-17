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

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#include "test/end2end/unary/rpcz/rpcz_fixture.h"

#include "test/end2end/common/util.h"
#include "test/end2end/unary/rpcz/real_server.h"
#include "test/end2end/unary/rpcz/rpcz_server.h"

int test_argc;
char** client_argv;
char** proxy_argv;
char** real_server_argv;

namespace trpc::testing {

std::shared_ptr<trpc::http::HttpServiceProxy> TestRpcz::http_proxy_;
std::shared_ptr<trpc::testing::rpcz::RpczServiceProxy> TestRpcz::rpcz_proxy_;
std::unique_ptr<TestSignaller> TestRpcz::proxy_test_signal_;
std::unique_ptr<TestSignaller> TestRpcz::real_server_test_signal_;
std::unique_ptr<SubProcess> TestRpcz::proxy_process_;
std::unique_ptr<SubProcess> TestRpcz::real_server_process_;

void TestRpcz::SetUpTestSuite() {
  real_server_test_signal_ = std::make_unique<TestSignaller>();
  real_server_process_ = std::make_unique<SubProcess>([] {
    RealServer real_server(real_server_test_signal_.get());
    real_server.Main(test_argc, real_server_argv);
    real_server.Wait();
  });

  proxy_test_signal_ = std::make_unique<TestSignaller>();
  proxy_process_ = std::make_unique<SubProcess>([] {
    RpczServer rpcz_server(proxy_test_signal_.get());
    rpcz_server.Main(test_argc, proxy_argv);
    rpcz_server.Wait();
  });

  real_server_test_signal_->ClientWaitToContinue();
  proxy_test_signal_->ClientWaitToContinue();

  InitializeRuntimeEnv(test_argc, client_argv);
  http_proxy_ = trpc::GetTrpcClient()->GetProxy<::trpc::http::HttpServiceProxy>("http_client");
  rpcz_proxy_ = trpc::GetTrpcClient()->GetProxy<::trpc::testing::rpcz::RpczServiceProxy>("rpcz_client");
}

void TestRpcz::TearDownTestSuite() {
  DestroyRuntimeEnv();
  proxy_process_.reset();
  real_server_process_.reset();
}

}  // namespace trpc::testing
#endif
