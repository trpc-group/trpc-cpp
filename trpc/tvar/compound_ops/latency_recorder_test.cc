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

#include "trpc/tvar/compound_ops/latency_recorder.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

namespace {

using trpc::tvar::LatencyRecorder;

}  // namespace

namespace trpc::testing {

/// @brief Test fixture to load config.
class TestLatencyRecorder : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    ASSERT_EQ(trpc::TrpcConfig::GetInstance()->Init("trpc/tvar/testing/series.yaml"), 0);
  }

  static void TearDownTestCase() {}
};

/// @brief Test in single thread.
TEST_F(TestLatencyRecorder, Test) {
  LatencyRecorder recorder(100);
  constexpr int N = 10000;

  for (int i = 0; i < N; ++i) {
    recorder.Update(i + 1);
  }

  std::this_thread::sleep_for(std::chrono::seconds(3));

  ASSERT_LE(recorder.QPS(), 10000);
  ASSERT_LE(recorder.QPS(100), 10000);
  std::cout << "QPS(100)=" << recorder.QPS(100) << std::endl;
  ASSERT_EQ(recorder.Latency(), 5000);
  ASSERT_EQ(recorder.Latency(100), 5000);
  ASSERT_EQ(recorder.MaxLatency(), 10000);
  ASSERT_EQ(recorder.Count(), 10000);
  for (int k = 1; k <= 10; ++k) {
    auto value = recorder.LatencyPercentile(k / 10.0);
    ASSERT_GT(value, (k - 1) * 1000);
    ASSERT_LT(value, (k + 1) * 1000);
    std::cout << "k=" << k << " value=" << value << std::endl;
  }
}

/// @brief Test in multiple threads.
TEST_F(TestLatencyRecorder, MultiThreads) {
  std::vector<std::thread> v;
  constexpr int ThreadNums = 10;
  v.reserve(ThreadNums);
  LatencyRecorder recorder(100);
  for (int i = 1; i <= ThreadNums; ++i) {
    v.emplace_back([a = i, &recorder]() {
      for (int i = 0; i < 1000; ++i) {
        recorder.Update(i * 10 + a);
      }
    });
  }
  for (auto&& t : v) {
    t.join();
  }

  std::this_thread::sleep_for(std::chrono::seconds(3));

  ASSERT_LE(recorder.QPS(), 10000);
  ASSERT_LE(recorder.QPS(100), 10000);
  std::cout << "QPS(100)=" << recorder.QPS(100) << std::endl;
  ASSERT_EQ(recorder.Latency(), 5000);
  ASSERT_EQ(recorder.Latency(100), 5000);
  ASSERT_EQ(recorder.MaxLatency(), 10000);
  ASSERT_EQ(recorder.Count(), 10000);

  for (int k = 1; k <= 10; ++k) {
    auto value = recorder.LatencyPercentile(k / 10.0);
    ASSERT_GT(value, (k - 1) * 1000);
    ASSERT_LT(value, (k + 1) * 1000);
    std::cout << "k=" << k << " value=" << value << std::endl;
  }
}

/// @brief Test for exposed LatencyRecorder.
TEST_F(TestLatencyRecorder, Exposed) {
  LatencyRecorder recorder("l");

  auto parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/a/b");
  LatencyRecorder recorder2(parent, "l2");

  ASSERT_TRUE(trpc::tvar::TrpcVarGroup::TryGet("/l", true)->isObject());
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/l/qps")->asInt(), 0);
  ASSERT_TRUE(trpc::tvar::TrpcVarGroup::TryGet("/l/qps", true)->isMember("now"));
  ASSERT_TRUE(trpc::tvar::TrpcVarGroup::TryGet("/a/b/l2", true)->isObject());
  ASSERT_TRUE(trpc::tvar::TrpcVarGroup::TryGet("/a/b/l2/qps", true)->isObject());
  ASSERT_TRUE(trpc::tvar::TrpcVarGroup::TryGet("/a/b/l2/qps", true)->isMember("now"));
}

/// @brief Test for not abort on same path.
TEST_F(TestLatencyRecorder, NotAbortOnSamePath) {
  trpc::tvar::SetWhetherToAbortOnSamePath(false);
  auto recorder = std::make_unique<LatencyRecorder>("l");
  ASSERT_TRUE(recorder->IsExposed());
  ASSERT_TRUE(trpc::tvar::TrpcVarGroup::TryGet("/l", true)->isObject());
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/l/qps")->asInt(), 0);
  ASSERT_TRUE(trpc::tvar::TrpcVarGroup::TryGet("/l/qps", true)->isMember("now"));
  auto repetitive_recorder = std::make_unique<LatencyRecorder>("l");
  ASSERT_FALSE(repetitive_recorder->IsExposed());
  recorder.reset();
  recorder = std::make_unique<LatencyRecorder>("l");
  ASSERT_TRUE(recorder->IsExposed());
  ASSERT_TRUE(trpc::tvar::TrpcVarGroup::TryGet("/l", true)->isObject());
  ASSERT_EQ(trpc::tvar::TrpcVarGroup::TryGet("/l/qps")->asInt(), 0);
  ASSERT_TRUE(trpc::tvar::TrpcVarGroup::TryGet("/l/qps", true)->isMember("now"));
}

}  // namespace trpc::testing
