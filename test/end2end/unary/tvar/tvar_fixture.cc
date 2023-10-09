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

#include "test/end2end/unary/tvar/tvar_fixture.h"

#include "test/end2end/common/util.h"
#include "test/end2end/unary/tvar/tvar_server.h"

int test_argc;
char** client_argv;
char** server_argv;

namespace trpc::testing {

std::shared_ptr<trpc::http::HttpServiceProxy> TestTvar::http_proxy_;
std::shared_ptr<trpc::testing::tvar::TvarServiceProxy> TestTvar::tvar_proxy_;
std::unique_ptr<TestSignaller> TestTvar::test_signal_;
std::unique_ptr<SubProcess> TestTvar::server_process_;

void TestTvar::SetUpTestSuite() {
  test_signal_ = std::make_unique<TestSignaller>();
  server_process_ = std::make_unique<SubProcess>([] {
    TvarServer tvar_server(test_signal_.get());
    tvar_server.Main(test_argc, server_argv);
    tvar_server.Wait();
  });

  test_signal_->ClientWaitToContinue();

  // Let server take samples before tests start.
  std::this_thread::sleep_for(std::chrono::seconds(10));

  InitializeRuntimeEnv(test_argc, client_argv);
  http_proxy_ = trpc::GetTrpcClient()->GetProxy<::trpc::http::HttpServiceProxy>("http_client");
  tvar_proxy_ = trpc::GetTrpcClient()->GetProxy<::trpc::testing::tvar::TvarServiceProxy>("tvar_client");
}

void TestTvar::TearDownTestSuite() {
  DestroyRuntimeEnv();
  server_process_.reset();
}

}  // namespace trpc::testing
