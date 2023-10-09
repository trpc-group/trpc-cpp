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

#include "rapidjson/document.h"
#include "test/end2end/unary/tvar/tvar_fixture.h"
#include "trpc/client/client_context.h"
#include "trpc/client/make_client_context.h"

namespace trpc::testing {

namespace {

#define TEST_RPC_FUNC(func, expected_msg) \
  TestRequest req; \
  TestReply rsp; \
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(TestTvar::tvar_proxy_); \
  trpc::Status status = func(ctx, req, &rsp); \
  ASSERT_TRUE(status.OK()); \
  ASSERT_EQ(rsp.msg(), expected_msg)

struct OpEQ {
  void operator()(std::string rsp, std::string expected_rsp) const {
    ASSERT_EQ(rsp, expected_rsp);
  }
};

struct OpGT {
  void operator()(std::string rsp, std::string expected_rsp) const {
    ASSERT_GT(rsp, expected_rsp);
  }
};

struct OpLE {
  void operator()(std::string rsp, std::string expected_rsp) const {
    int rsp_v = atoi(rsp.c_str());
    int exp_v = atoi(expected_rsp.c_str());
    ASSERT_LE(rsp_v, exp_v);
  }
};

using TestRequest = trpc::testing::tvar::TestRequest;
using TestReply = trpc::testing::tvar::TestReply;

constexpr const char kAdminUrlBase[] = "http://www.testtvar.com/cmds/var/";

template <typename Op>
void TestHttpGetString(std::string tail_url, std::string expected_rsp) {
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(TestTvar::http_proxy_);
  std::string url = std::string(kAdminUrlBase) + tail_url;
  std::string rsp;
  trpc::Status status = TestTvar::http_proxy_->GetString(ctx, url, &rsp);
  Op()(rsp, expected_rsp);
}

void TestHttpGetJson(std::string tail_url, const char* expected_key) {
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(TestTvar::http_proxy_);
  std::string url = std::string(kAdminUrlBase) + tail_url;
  rapidjson::Document rsp;
  trpc::Status status = TestTvar::http_proxy_->Get(ctx, url, &rsp);
  ASSERT_TRUE(status.OK());
  ASSERT_TRUE(rsp.HasMember(expected_key));
}

}  // namespace

// 1. Server has exposed Counter var.
// 2. Successfully get current value.
TEST_F(TestTvar, GetCounterCurrentValueOk) {
  TestHttpGetString<OpEQ>("counter/normal", "0");
}

// 1. Server has exposed Counter var.
// 2. Successfully get history value.
TEST_F(TestTvar, GetCounterHistoryValueOk) {
  TestHttpGetJson("counter/normal?history=true", "now");
}

// 1. Server has dynamic Counter var.
// 2. Client send rpc request to trigger var exposed generation.
// 3. Successfully get current value.
TEST_F(TestTvar, GetCounterDynamic) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireDynamicCounter, "");
  TestHttpGetString<OpEQ>("counter/dynamic", "0");
}

// 1. Server has exposed Counter var.
// 2. Client send rpc request to trigger var Add operation.
// 3. Successfully get current value.
TEST_F(TestTvar, GetCounterAdd) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireCounterAdd, "");
  TestHttpGetString<OpEQ>("counter/add", "2");
}

// 1. Server has exposed Counter var.
// 2. Client send rpc request to trigger var Increment operation.
// 3. Successfully get current value.
TEST_F(TestTvar, GetCounterIncrement) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireCounterIncrement, "");
  TestHttpGetString<OpEQ>("counter/increment", "1");
}

// 1. Server has exposed Counter var.
// 2. Client send rpc request to get absolute path.
TEST_F(TestTvar, GetCounterPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetCounterPath, "/counter/normal");
}

// 1. Server has not-exposed Counter var.
// 2. Failed to get current value.
TEST_F(TestTvar, GetCounterNotExposedValue) {
  TestHttpGetJson("counter/notexposed", "message");
}

// 1. Server has exposed Counter var, but already destruted.
// 2. Failed to get current value.
TEST_F(TestTvar, GetCounterDestrutedValue) {
  TestHttpGetJson("counter/tmp", "message");
}

// 1. Server has not-exposed Counter var.
// 2. Get empty absolute path.
TEST_F(TestTvar, GetCounterNotExposedPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetCounterNotExposedPath, "");
}

TEST_F(TestTvar, GetGaugeCurrentValueOk) {
  TestHttpGetString<OpEQ>("gauge/normal", "0");
}

TEST_F(TestTvar, GetGaugeHistoryValueOk) {
  TestHttpGetJson("gauge/normal?history=true", "now");
}

TEST_F(TestTvar, GetGaugeDynamic) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireDynamicGauge, "");
  TestHttpGetString<OpEQ>("gauge/dynamic", "0");
}

TEST_F(TestTvar, GetGaugeAdd) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireGaugeAdd, "");
  TestHttpGetString<OpEQ>("gauge/add", "2");
}

TEST_F(TestTvar, GetGaugeSubtract) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireGaugeSubtract, "");
  TestHttpGetString<OpEQ>("gauge/subtract", "5");
}

TEST_F(TestTvar, GetGaugeIncrement) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireGaugeIncrement, "");
  TestHttpGetString<OpEQ>("gauge/increment", "1");
}

TEST_F(TestTvar, GetGaugeDecrement) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireGaugeDecrement, "");
  TestHttpGetString<OpEQ>("gauge/decrement", "9");
}

TEST_F(TestTvar, GetGaugePath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetGaugePath, "/gauge/normal");
}

TEST_F(TestTvar, GetGaugeNotExposedValue) {
  TestHttpGetJson("gauge/notexposed", "message");
}

TEST_F(TestTvar, GetGaugeDestrutedValue) {
  TestHttpGetJson("gauge/tmp", "message");
}

TEST_F(TestTvar, GetGaugeNotExposedPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetGaugeNotExposedPath, "");
}

TEST_F(TestTvar, GetMinerCurrentValueOk) {
  TestHttpGetString<OpEQ>("miner/normal", "100");
}

TEST_F(TestTvar, GetMinerDynamic) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireDynamicMiner, "");
  TestHttpGetString<OpEQ>("miner/dynamic", "100");
}

TEST_F(TestTvar, GetMinerUpdate) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireMinerUpdate, "");
  TestHttpGetString<OpEQ>("miner/update", "10");
}

TEST_F(TestTvar, GetMinerNotUpdate) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireMinerNotUpdate, "");
  TestHttpGetString<OpEQ>("miner/notupdate", "10");
}

TEST_F(TestTvar, GetMinerPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetMinerPath, "/miner/normal");
}

TEST_F(TestTvar, GetMinerNotExposedValue) {
  TestHttpGetJson("miner/notexposed", "message");
}

TEST_F(TestTvar, GetMinerDestrutedValue) {
  TestHttpGetJson("miner/tmp", "message");
}

TEST_F(TestTvar, GetMinerNotExposedPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetMinerNotExposedPath, "");
}

TEST_F(TestTvar, GetMaxerCurrentValueOk) {
  TestHttpGetString<OpEQ>("maxer/normal", "0");
}

TEST_F(TestTvar, GetMaxerDynamic) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireDynamicMaxer, "");
  TestHttpGetString<OpEQ>("maxer/dynamic", "0");
}

TEST_F(TestTvar, GetMaxerUpdate) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireMaxerUpdate, "");
  TestHttpGetString<OpEQ>("maxer/update", "10");
}

TEST_F(TestTvar, GetMaxerNotUpdate) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireMinerNotUpdate, "");
  TestHttpGetString<OpEQ>("maxer/notupdate", "100");
}

TEST_F(TestTvar, GetMaxerPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetMaxerPath, "/maxer/normal");
}

TEST_F(TestTvar, GetMaxerNotExposedValue) {
  TestHttpGetJson("maxer/notexposed", "message");
}

TEST_F(TestTvar, GetMaxerDestrutedValue) {
  TestHttpGetJson("maxer/tmp", "message");
}

TEST_F(TestTvar, GetMaxerNotExposedPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetMaxerNotExposedPath, "");
}

TEST_F(TestTvar, GetStatusCurrentValueOk) {
  TestHttpGetString<OpEQ>("status/normal", "0");
}

TEST_F(TestTvar, GetStatusHistoryValueOk) {
  TestHttpGetJson("status/normal?history=true", "now");
}

TEST_F(TestTvar, GetStatusDynamic) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireDynamicStatus, "");
  TestHttpGetString<OpEQ>("status/dynamic", "0");
}

TEST_F(TestTvar, GetStatusUpdate) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireStatusUpdate, "");
  TestHttpGetString<OpEQ>("status/update", "10");
}

TEST_F(TestTvar, GetStatusPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetStatusPath, "/status/normal");
}

TEST_F(TestTvar, GetStatusNotExposedValue) {
  TestHttpGetJson("status/notexposed", "message");
}

TEST_F(TestTvar, GetStatusDestrutedValue) {
  TestHttpGetJson("status/tmp", "message");
}

TEST_F(TestTvar, GetStatusNotExposedPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetStatusNotExposedPath, "");
}

TEST_F(TestTvar, GetPassiveStatusCurrentValueOk) {
  TestHttpGetString<OpEQ>("passivestatus/normal", "0");
}

TEST_F(TestTvar, GetPassiveStatusHistoryValueOk) {
  TestHttpGetJson("passivestatus/normal?history=true", "now");
}

TEST_F(TestTvar, GetPassiveStatusDynamic) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireDynamicPassiveStatus, "");
  TestHttpGetString<OpEQ>("passivestatus/dynamic", "0");
}

TEST_F(TestTvar, GetPassiveStatusPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetPassiveStatusPath, "/passivestatus/normal");
}

TEST_F(TestTvar, GetPassiveStatusNotExposedValue) {
  TestHttpGetJson("passivestatus/notexposed", "message");
}

TEST_F(TestTvar, GetPassiveStatusDestrutedValue) {
  TestHttpGetJson("passivestatus/tmp", "message");
}

TEST_F(TestTvar, GetPassiveStatusNotExposedPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetPassiveStatusNotExposedPath, "");
}

TEST_F(TestTvar, GetAveragerCurrentValueOk) {
  TestHttpGetString<OpEQ>("averager/normal", "0");
}

TEST_F(TestTvar, GetAveragerDynamic) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireDynamicAverager, "");
  TestHttpGetString<OpEQ>("averager/dynamic", "0");
}

TEST_F(TestTvar, GetAveragerUpdate) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireAveragerUpdate, "");
  TestHttpGetString<OpEQ>("averager/update", "50");
}

TEST_F(TestTvar, GetAveragerPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetAveragerPath, "/averager/normal");
}

TEST_F(TestTvar, GetAveragerNotExposedValue) {
  TestHttpGetJson("averager/notexposed", "message");
}

TEST_F(TestTvar, GetAveragerDestrutedValue) {
  TestHttpGetJson("averager/tmp", "message");
}

TEST_F(TestTvar, GetAveragerNotExposedPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetAveragerNotExposedPath, "");
}

TEST_F(TestTvar, GetIntRecorderCurrentValueOk) {
  TestHttpGetString<OpEQ>("intrecorder/normal", "0");
}

TEST_F(TestTvar, GetIntRecorderDynamic) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireDynamicIntRecorder, "");
  TestHttpGetString<OpEQ>("intrecorder/dynamic", "0");
}

TEST_F(TestTvar, GetIntRecorderUpdate) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireIntRecorderUpdate, "");
  TestHttpGetString<OpEQ>("intrecorder/update", "50");
}

TEST_F(TestTvar, GetIntRecorderPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetIntRecorderPath, "/intrecorder/normal");
}

TEST_F(TestTvar, GetIntRecorderNotExposedValue) {
  TestHttpGetJson("intrecorder/notexposed", "message");
}

TEST_F(TestTvar, GetIntRecorderDestrutedValue) {
  TestHttpGetJson("intrecorder/tmp", "message");
}

TEST_F(TestTvar, GetIntRecorderNotExposedPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetIntRecorderNotExposedPath, "");
}

TEST_F(TestTvar, GetWindowCurrentValueOk) {
  TestHttpGetString<OpEQ>("window/normal", "0");
}

TEST_F(TestTvar, GetWindowHistoryValueOk) {
  TestHttpGetJson("window/normal?history=true", "now");
}

TEST_F(TestTvar, GetWindowDynamic) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireDynamicWindowCounter, "");
  TestHttpGetString<OpEQ>("window/dynamic", "0");
}

TEST_F(TestTvar, GetWindowPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetWindowCounterPath, "/window/normal");
}

TEST_F(TestTvar, GetWindowNotExposedValue) {
  TestHttpGetJson("window/notexposed", "message");
}

TEST_F(TestTvar, GetWindowDestrutedValue) {
  TestHttpGetJson("window/tmp", "message");
}

TEST_F(TestTvar, GetWindowNotExposedPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetWindowCounterNotExposedPath, "");
}

TEST_F(TestTvar, GetPerSecondCurrentValueOk) {
  TestHttpGetString<OpEQ>("persecond/normal", "0");
}

TEST_F(TestTvar, GetPerSecondHistoryValueOk) {
  TestHttpGetJson("persecond/normal?history=true", "now");
}

TEST_F(TestTvar, GetPerSecondDynamic) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireDynamicPerSecondCounter, "");
  TestHttpGetString<OpEQ>("persecond/dynamic", "0");
}

TEST_F(TestTvar, GetPerSecondPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetPerSecondCounterPath, "/persecond/normal");
}

TEST_F(TestTvar, GetPerSecondNotExposedValue) {
  TestHttpGetJson("persecond/notexposed", "message");
}

TEST_F(TestTvar, GetPerSecondDestrutedValue) {
  TestHttpGetJson("persecond/tmp", "message");
}

TEST_F(TestTvar, GetPerSecondNotExposedPath) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->GetPerSecondCounterNotExposedPath, "");
}

TEST_F(TestTvar, GetLatencyRecorderDynamic) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireDynamicLatencyRecorder, "");
  TestHttpGetString<OpEQ>("latencyrecorder/dynamic/count", "0");
}

TEST_F(TestTvar, GetLatencyRecorderUpdate) {
  TEST_RPC_FUNC(TestTvar::tvar_proxy_->FireLatencyRecorderUpdate, "");

  // Let server take samples.
  std::this_thread::sleep_for(std::chrono::seconds(2));

  TestHttpGetString<OpEQ>("latencyrecorder/update/latency", "5000");
  TestHttpGetString<OpEQ>("latencyrecorder/update/max_latency", "10000");
  TestHttpGetString<OpLE>("latencyrecorder/update/qps", "1000");
  TestHttpGetString<OpEQ>("latencyrecorder/update/count", "10000");
  TestHttpGetString<OpGT>("latencyrecorder/update/latency_80", "7000");
  TestHttpGetString<OpGT>("latencyrecorder/update/latency_999", "9000");
  TestHttpGetString<OpGT>("latencyrecorder/update/latency_9999", "9000");
}

}  // namespace trpc::testing

int main(int argc, char** argv) {
  if (!trpc::testing::ExtractServerAndClientArgs(argc, argv, &test_argc, &client_argv, &server_argv)) {
    exit(EXIT_FAILURE);
  }

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
