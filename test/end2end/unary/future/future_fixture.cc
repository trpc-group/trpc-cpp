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

#include "test/end2end/unary/future/future_fixture.h"

#include "test/end2end/common/util.h"
#include "test/end2end/unary/future/future_server.h"

int test_argc;
char** client_argv;
char** server_argv;

namespace trpc::testing {

namespace {

using FutureServiceProxy = TestFuture::FutureServiceProxy;

}  // namespace

std::shared_ptr<FutureServiceProxy> TestFuture::tcp_complex_proxy_;
std::shared_ptr<FutureServiceProxy> TestFuture::tcp_complex_proxy_not_exist_;
std::shared_ptr<FutureServiceProxy> TestFuture::udp_complex_proxy_;
std::shared_ptr<FutureServiceProxy> TestFuture::tcp_conn_pool_proxy_;
std::shared_ptr<FutureServiceProxy> TestFuture::tcp_conn_pool_proxy_not_exist_;
std::shared_ptr<FutureServiceProxy> TestFuture::udp_conn_pool_proxy_;
std::unique_ptr<TestSignaller> TestFuture::test_signal_;
std::unique_ptr<SubProcess> TestFuture::server_process_;

void TestFuture::SetUpTestSuite() {
  test_signal_ = std::make_unique<TestSignaller>();
  server_process_ = std::make_unique<SubProcess>([] {
    FutureServer future_server(test_signal_.get());
    future_server.Main(test_argc, server_argv);
    future_server.Wait();
  });

  test_signal_->ClientWaitToContinue();

  InitializeRuntimeEnv(test_argc, client_argv);
  tcp_complex_proxy_ = trpc::GetTrpcClient()->GetProxy<FutureServiceProxy>("tcp_complex");
  tcp_complex_proxy_not_exist_ = trpc::GetTrpcClient()->GetProxy<FutureServiceProxy>("tcp_complex_not_exist");
  udp_complex_proxy_ = trpc::GetTrpcClient()->GetProxy<FutureServiceProxy>("udp_complex");
  tcp_conn_pool_proxy_ = trpc::GetTrpcClient()->GetProxy<FutureServiceProxy>("tcp_conn_pool");
  tcp_conn_pool_proxy_not_exist_ = trpc::GetTrpcClient()->GetProxy<FutureServiceProxy>("tcp_conn_pool_not_exist");
  udp_conn_pool_proxy_ = trpc::GetTrpcClient()->GetProxy<FutureServiceProxy>("udp_conn_pool");
}

void TestFuture::TearDownTestSuite() {
  DestroyRuntimeEnv();
  server_process_.reset();
}

}  // namespace trpc::testing
