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
#include "trpc/client/client_context.h"
#include "trpc/client/make_client_context.h"

namespace trpc::testing {

namespace {

constexpr const char kAdminUrlBase[] = "http://www.testtvar.com/cmds/var/";

void TestHistoryValue(std::string tail_url, std::string expected_value) {
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(TestTvar::http_proxy_);
  std::string url = std::string(kAdminUrlBase) + tail_url + "?history=true";
  std::string rsp;
  trpc::Status status = TestTvar::http_proxy_->GetString(ctx, url, &rsp);
  ASSERT_TRUE(status.OK());
  ASSERT_EQ(rsp, expected_value);
}

}  // namespace

// 1. Server has exposed var.
// 2. Failed to get history value due to config not enabled.
TEST_F(TestTvar, counter) {
  TestHistoryValue("counter/normal", "0");
}

TEST_F(TestTvar, gauge) {
  TestHistoryValue("gauge/normal", "0");
}

TEST_F(TestTvar, miner) {
  TestHistoryValue("miner/normal", "100");
}

TEST_F(TestTvar, maxer) {
  TestHistoryValue("maxer/normal", "0");
}

TEST_F(TestTvar, status) {
  TestHistoryValue("status/normal", "0");
}

TEST_F(TestTvar, passivestatus) {
  TestHistoryValue("passivestatus/normal", "0");
}

TEST_F(TestTvar, averager) {
  TestHistoryValue("averager/normal", "0");
}

TEST_F(TestTvar, intrecorder) {
  TestHistoryValue("intrecorder/normal", "0");
}

TEST_F(TestTvar, window) {
  TestHistoryValue("window/normal", "0");
}

TEST_F(TestTvar, persecond) {
  TestHistoryValue("persecond/normal", "0");
}

}  // namespace trpc::testing

int main(int argc, char** argv) {
  if (!trpc::testing::ExtractServerAndClientArgs(argc, argv, &test_argc, &client_argv, &server_argv)) {
    exit(EXIT_FAILURE);
  }

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
