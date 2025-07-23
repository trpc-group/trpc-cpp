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
// Basic categories with series close in config are tested here.

#include <atomic>
#include <memory>
#include <thread>

#include "gtest/gtest.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/tvar/basic_ops/passive_status.h"
#include "trpc/tvar/basic_ops/recorder.h"
#include "trpc/tvar/basic_ops/reducer.h"
#include "trpc/tvar/basic_ops/status.h"

namespace {

using trpc::tvar::GetSystemMicroseconds;
using trpc::tvar::OpAdd;
using trpc::tvar::OpMax;
using trpc::tvar::OpMin;
using trpc::tvar::OpMinus;
using trpc::tvar::Averager;
using trpc::tvar::Counter;
using trpc::tvar::Gauge;
using trpc::tvar::IntRecorder;
using trpc::tvar::Maxer;
using trpc::tvar::Miner;
using trpc::tvar::PassiveStatus;
using trpc::tvar::Status;

}  // namespace

namespace trpc::testing {

/// @brief Test fixture to load config.
class TestTvarBasicOpsNoSeries : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    ASSERT_EQ(trpc::TrpcConfig::GetInstance()->Init("trpc/tvar/testing/noseries.yaml"), 0);
  }

  static void TearDownTestCase() {}
};

/// @brief Test for PassiveStatus.
TEST_F(TestTvarBasicOpsNoSeries, PassiveStatus) {
  std::atomic_int64_t ret{0};
  PassiveStatus<int> st("passive_status", [&ret]() { return ret.load(std::memory_order_relaxed); });
  ASSERT_EQ(st.GetAbsPath(), "/passive_status");

  auto parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/a/b");
  PassiveStatus<int> st2(parent, "passive_status",
                         [&ret]() { return ret.load(std::memory_order_relaxed); });
  ASSERT_EQ(st2.GetAbsPath(), "/a/b/passive_status");

  ret = 10;
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/passive_status")->asInt(), ret);
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/a/b/passive_status")->asInt(), ret);
}

/// @brief Test for Status.
TEST_F(TestTvarBasicOpsNoSeries, Status) {
  Status<int> st("status", 9);
  ASSERT_EQ(st.GetAbsPath(), "/status");

  auto parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/a/b");
  Status<int> st2(parent, "status", 9);
  ASSERT_EQ(st2.GetAbsPath(), "/a/b/status");

  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/status")->asInt(), 9);
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/a/b/status")->asInt(), 9);
}

/// @brief Test for Recorder.
TEST_F(TestTvarBasicOpsNoSeries, Recorder) {
  Averager<int> averager("averager");
  ASSERT_EQ(averager.GetAbsPath(), "/averager");

  IntRecorder int_recorder("int_recorder");
  ASSERT_EQ(int_recorder.GetAbsPath(), "/int_recorder");

  auto parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/a/b");
  Averager<int> averager2(parent, "averager");
  ASSERT_EQ(averager2.GetAbsPath(), "/a/b/averager");
  IntRecorder int_recorder2(parent, "int_recorder");
  ASSERT_EQ(int_recorder2.GetAbsPath(), "/a/b/int_recorder");

  for (int i = 0; i < 100; ++i) {
    averager.Update(i);
    averager2.Update(i);
    int_recorder.Update(i);
    int_recorder2.Update(i);
  }

  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/averager")->asInt(), (99 * 50) / 100);
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/a/b/averager")->asInt(), (99 * 50) / 100);
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/int_recorder")->asInt(), (99 * 50) / 100);
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/a/b/int_recorder")->asInt(), (99 * 50) / 100);
}

/// @brief Test for Reducer.
TEST_F(TestTvarBasicOpsNoSeries, Reducer) {
  Counter<int> counter("count");
  ASSERT_EQ(counter.GetAbsPath(), "/count");
  Gauge<int> gauge("gauge");
  ASSERT_EQ(gauge.GetAbsPath(), "/gauge");
  Maxer<int> maxer("maxer");
  ASSERT_EQ(maxer.GetAbsPath(), "/maxer");
  Miner<int> miner("miner");
  ASSERT_EQ(miner.GetAbsPath(), "/miner");

  auto parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/a/b");
  Counter<int> counter2(parent, "count");
  ASSERT_EQ(counter2.GetAbsPath(), "/a/b/count");
  Gauge<int> gauge2(parent, "gauge");
  ASSERT_EQ(gauge2.GetAbsPath(), "/a/b/gauge");
  Maxer<int> maxer2(parent, "maxer");
  ASSERT_EQ(maxer2.GetAbsPath(), "/a/b/maxer");
  Miner<int> miner2(parent, "miner");
  ASSERT_EQ(miner2.GetAbsPath(), "/a/b/miner");

  for (int i = 0; i < 100; ++i) {
    counter.Update(i);
    counter2.Update(i);
    gauge.Update(i);
    gauge2.Update(i);
    maxer.Update(i);
    maxer2.Update(i);
    miner.Update(i);
    miner2.Update(i);
  }

  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/count")->asInt(), 99 * 50);
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/a/b/count")->asInt(), 99 * 50);
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/gauge")->asInt(), 99 * 50);
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/a/b/gauge")->asInt(), 99 * 50);
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/maxer")->asInt(), 99);
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/a/b/maxer")->asInt(), 99);
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/miner")->asInt(), 0);
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/a/b/miner")->asInt(), 0);
}

}  // namespace trpc::testing
