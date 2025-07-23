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

#include "trpc/tvar/basic_ops/reducer.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

#include "trpc/tvar/common/op_util.h"
#include "trpc/tvar/common/tvar_group.h"

namespace {

using trpc::tvar::GetSystemMicroseconds;
using trpc::tvar::OpAdd;
using trpc::tvar::OpMax;
using trpc::tvar::OpMin;
using trpc::tvar::OpMinus;
using trpc::tvar::Counter;
using trpc::tvar::Gauge;
using trpc::tvar::Maxer;
using trpc::tvar::Miner;

/// @brief Validate series value format.
bool ValidateSeries(std::optional<Json::Value> gauge_value) {
  return gauge_value.has_value() && gauge_value->isObject() && gauge_value->isMember("now") &&
         gauge_value->isMember("latest_sec") && gauge_value->isMember("latest_min") &&
         gauge_value->isMember("latest_hour") && gauge_value->isMember("latest_day");
}

}  // namespace

namespace trpc::testing {

/// @brief Text fixture to load comfig.
class TestReducer : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    ASSERT_EQ(trpc::TrpcConfig::GetInstance()->Init("trpc/tvar/testing/series.yaml"), 0);
  }

  static void TearDownTestCase() {}
};

/// @brief Test for op util.
TEST_F(TestReducer, TestOp) {
  // OpAdd
  {
    OpAdd<int64_t> OP;
    int64_t value{0};

    OP(&value, 10);
    ASSERT_EQ(10, value);

    OP(&value, -11);
    ASSERT_EQ(-1, value);
  }

  // OpMinus
  {
    OpMinus<int64_t> OP;
    std::int64_t value{0};

    OP(&value, -10);
    ASSERT_EQ(10, value);

    OP(&value, 11);
    ASSERT_EQ(-1, value);
  }

  // OpMin
  {
    OpMin<int64_t> Op;
    int64_t value{0};

    Op(&value, 1);
    ASSERT_EQ(0, value);

    Op(&value, -1);
    ASSERT_EQ(-1, value);

    Op(&value, 100);
    ASSERT_EQ(-1, value);
  }

  // OpMax
  {
    OpMax<int64_t> Op;
    int64_t value{0};

    Op(&value, 1);
    ASSERT_EQ(1, value);

    Op(&value, -1);
    ASSERT_EQ(1, value);

    Op(&value, 100);
    ASSERT_EQ(100, value);
  }
}

/// @brief Test for Counter type.
TEST_F(TestReducer, TestCounter) {
  Counter<uint64_t> counter1;
  counter1.Add(2);
  counter1.Update(4);
  counter1.Increment();
  ASSERT_EQ(7, counter1.GetValue());

  Counter<double> counter2;
  counter2.Add(2.0);
  counter2.Update(4.0);
  counter2.Increment();
  ASSERT_EQ(7.0, counter2.GetValue());
}

/// @brief Test for Gauge type.
TEST_F(TestReducer, TestGauge) {
  Gauge<uint64_t> gauge1;
  gauge1.Add(2);
  gauge1.Update(4);
  gauge1.Increment();
  ASSERT_EQ(7, gauge1.GetValue());

  Gauge<double> gauge2;
  gauge2.Add(2.0);
  gauge2.Update(4.0);
  gauge2.Increment();
  ASSERT_EQ(7.0, gauge2.GetValue());

  Gauge<int64_t> gauge3;
  gauge3.Subtract(4);
  gauge3.Update(-3);
  gauge3.Decrement();
  ASSERT_EQ(-8, gauge3.GetValue());
}

namespace {

constexpr int kThreadsNum = 4;
constexpr int kOpsPerThread = 5000;

}  // namespace

/// @brief Compare Gauge with traditional atomic.
TEST_F(TestReducer, GaugePerf) {
  std::atomic_uint64_t total_time{0};
  Gauge<uint64_t> gauge;
  std::vector<std::thread> vec;
  vec.reserve(kThreadsNum);
  for (int i = 0; i < kThreadsNum; ++i) {
    vec.emplace_back([&gauge, &total_time]() {
      auto begin_ts = GetSystemMicroseconds();
      for (int i = 0; i < kOpsPerThread; ++i) {
        gauge.Update(2);
      }
      auto end_ts = GetSystemMicroseconds();
      if (end_ts > begin_ts) {
        total_time += end_ts - begin_ts;
        // std::cout << total_time << std::endl;
      }
    });
  }
  for (auto& v : vec) {
    v.join();
  }
  ASSERT_EQ(2 * kThreadsNum * kOpsPerThread, gauge.GetValue());
  std::cout << "recorder takes " << total_time / kThreadsNum << "us per " << kOpsPerThread
            << " samples with " << kThreadsNum << " threads" << std::endl;

  vec.clear();
  total_time = 0;
  std::atomic_uint64_t gauge_atomic = 0;
  for (int i = 0; i < kThreadsNum; ++i) {
    vec.emplace_back([&gauge_atomic, &total_time]() {
      auto begin_ts = GetSystemMicroseconds();
      for (int i = 0; i < kOpsPerThread; ++i) {
        gauge_atomic.fetch_add(2, std::memory_order_relaxed);
      }
      auto end_ts = GetSystemMicroseconds();
      if (end_ts > begin_ts) {
        total_time += end_ts - begin_ts;
      }
    });
  }
  for (auto& v : vec) {
    v.join();
  }
  ASSERT_EQ(2ul * kThreadsNum * kOpsPerThread, gauge_atomic.load());
  std::cout << "recorder takes " << total_time / kThreadsNum << "us per " << kOpsPerThread
            << " samples with " << kThreadsNum << " threads" << std::endl;
}

/// @brief Test for miner.
TEST_F(TestReducer, TestMiner) {
  Miner<uint64_t> miner;
  ASSERT_EQ(std::numeric_limits<uint64_t>::max(), miner.GetValue());
  miner.Update(3);
  miner.Update(9);
  ASSERT_EQ(3, miner.GetValue());
  miner.Update(1);
  ASSERT_EQ(1, miner.GetValue());
  miner.Update(std::numeric_limits<uint64_t>::max());
  ASSERT_EQ(1, miner.GetValue());
  miner.Update(0);
  ASSERT_EQ(0, miner.GetValue());

  Miner<int64_t> miner2;
  ASSERT_EQ(std::numeric_limits<int64_t>::max(), miner2.GetValue());
  miner2.Update(3);
  miner2.Update(9);
  ASSERT_EQ(3, miner2.GetValue());
  miner2.Update(-1);
  ASSERT_EQ(-1, miner2.GetValue());
  miner2.Update(std::numeric_limits<int64_t>::max());
  ASSERT_EQ(-1, miner2.GetValue());
  miner2.Update(0);
  ASSERT_EQ(-1, miner2.GetValue());
  miner2.Update(std::numeric_limits<int64_t>::min());
  ASSERT_EQ(std::numeric_limits<int64_t>::min(), miner2.GetValue());
}

/// @brief Test for maxer.
TEST_F(TestReducer, TestMaxer) {
  Maxer<uint64_t> maxer;
  ASSERT_EQ(std::numeric_limits<uint64_t>::min(), maxer.GetValue());
  maxer.Update(3);
  maxer.Update(9);
  ASSERT_EQ(9, maxer.GetValue());
  maxer.Update(10);
  ASSERT_EQ(10, maxer.GetValue());
  maxer.Update(30);
  ASSERT_EQ(30, maxer.GetValue());

  Maxer<int64_t> maxer2;
  ASSERT_EQ(std::numeric_limits<int64_t>::min(), maxer2.GetValue());
  maxer2.Update(3);
  maxer2.Update(9);
  ASSERT_EQ(9, maxer2.GetValue());
  maxer2.Update(10);
  ASSERT_EQ(10, maxer2.GetValue());
  maxer2.Update(0);
  ASSERT_EQ(10, maxer2.GetValue());
  maxer2.Update(std::numeric_limits<int64_t>::max());
  ASSERT_EQ(std::numeric_limits<int64_t>::max(), maxer2.GetValue());
}

/// @brief Test for exposed.
TEST_F(TestReducer, Exposed) {
  std::weak_ptr<Maxer<uint64_t>::SeriesSamplerType> maxer_series_ptr;
  std::weak_ptr<Maxer<uint64_t>::ReducerSamplerType> maxer_reducer_ptr;
  std::weak_ptr<Miner<uint64_t>::SeriesSamplerType> miner_series_ptr;
  std::weak_ptr<Miner<uint64_t>::ReducerSamplerType> miner_reducer_ptr;
  std::weak_ptr<Gauge<uint64_t>::SeriesSamplerType> gauge_series_ptr;
  std::weak_ptr<Gauge<uint64_t>::ReducerSamplerType> gauge_reducer_ptr;
  std::weak_ptr<Counter<uint64_t>::SeriesSamplerType> counter_series_ptr;
  std::weak_ptr<Counter<uint64_t>::ReducerSamplerType> counter_reducer_ptr;

  {
    Maxer<uint64_t> maxer("user/maxer", 1);
    ASSERT_EQ(maxer.GetAbsPath(), "/user/maxer");
    maxer_series_ptr = maxer.GetSeriesSampler();
    ASSERT_FALSE(maxer_series_ptr.lock());
    maxer_reducer_ptr = maxer.GetReducerSampler();
    ASSERT_TRUE(maxer_reducer_ptr.lock());

    Miner<uint64_t> miner("user/miner", 1);
    ASSERT_EQ(miner.GetAbsPath(), "/user/miner");
    miner_series_ptr = miner.GetSeriesSampler();
    ASSERT_FALSE(miner_series_ptr.lock());
    miner_reducer_ptr = miner.GetReducerSampler();
    ASSERT_TRUE(miner_reducer_ptr.lock());

    Gauge<uint64_t> gauge("user/gauge", 1);
    ASSERT_EQ(gauge.GetAbsPath(), "/user/gauge");
    gauge_series_ptr = gauge.GetSeriesSampler();
    ASSERT_TRUE(gauge_series_ptr.lock());
    ASSERT_TRUE(ValidateSeries(tvar::TrpcVarGroup::TryGet("/user/gauge", true)));
    gauge_reducer_ptr = gauge.GetReducerSampler();
    ASSERT_TRUE(gauge_reducer_ptr.lock());

    Counter<uint64_t> counter("user/counter", 1);
    ASSERT_EQ(counter.GetAbsPath(), "/user/counter");
    counter_series_ptr = counter.GetSeriesSampler();
    ASSERT_TRUE(counter_series_ptr.lock());
    ASSERT_TRUE(ValidateSeries(tvar::TrpcVarGroup::TryGet("/user/counter", true)));
    counter_reducer_ptr = counter.GetReducerSampler();
    ASSERT_TRUE(counter_reducer_ptr.lock());
  }

  std::this_thread::sleep_for(std::chrono::seconds(3));

  ASSERT_FALSE(maxer_series_ptr.lock());
  ASSERT_FALSE(miner_series_ptr.lock());
  ASSERT_FALSE(gauge_series_ptr.lock());
  ASSERT_FALSE(counter_series_ptr.lock());

  ASSERT_FALSE(maxer_reducer_ptr.lock());
  ASSERT_FALSE(miner_reducer_ptr.lock());
  ASSERT_FALSE(gauge_reducer_ptr.lock());
  ASSERT_FALSE(counter_reducer_ptr.lock());
}

/// @brief Test for Maxer not abort on same path.
TEST_F(TestReducer, MaxerNotAbortOnSamePath) {
  const std::string kRelPath("user/maxer");
  const std::string kAbsPath("/user/maxer");
  // Cancel abort.
  trpc::tvar::SetWhetherToAbortOnSamePath(false);
  // Create exposed tvar.
  auto maxer = std::make_unique<Maxer<uint64_t>>(kRelPath, 1);
  ASSERT_TRUE(maxer->IsExposed());
  ASSERT_EQ(maxer->GetAbsPath(), kAbsPath);
  // Duplicate exposed.
  auto repetitive_maxer = std::make_unique<Maxer<uint64_t>>(kRelPath, 1);
  ASSERT_FALSE(repetitive_maxer->IsExposed());
  ASSERT_TRUE(repetitive_maxer->GetAbsPath().empty());
  // Cancel exposed tvar.
  maxer.reset();
  // Expose again.
  maxer = std::make_unique<Maxer<uint64_t>>(kRelPath, 1);
  ASSERT_TRUE(maxer->IsExposed());
  ASSERT_EQ(maxer->GetAbsPath(), kAbsPath);
}

/// @brief Test for Miner not abort on same path.
TEST_F(TestReducer, MinerNotAbortOnSamePath) {
  const std::string kRelPath("user/miner");
  const std::string kAbsPath("/user/miner");
  trpc::tvar::SetWhetherToAbortOnSamePath(false);
  auto miner = std::make_unique<Miner<uint64_t>>(kRelPath, 1);
  ASSERT_TRUE(miner->IsExposed());
  ASSERT_EQ(miner->GetAbsPath(), kAbsPath);
  auto repetitive_miner = std::make_unique<Miner<uint64_t>>(kRelPath, 1);
  ASSERT_FALSE(repetitive_miner->IsExposed());
  ASSERT_TRUE(repetitive_miner->GetAbsPath().empty());
  miner.reset();
  miner = std::make_unique<Miner<uint64_t>>(kRelPath, 1);
  ASSERT_TRUE(miner->IsExposed());
  ASSERT_EQ(miner->GetAbsPath(), kAbsPath);
}

/// @brief Test for gauge not abort on same path.
TEST_F(TestReducer, GaugeNotAbortOnSamePath) {
  const std::string kRelPath("user/gauge");
  const std::string kAbsPath("/user/gauge");
  trpc::tvar::SetWhetherToAbortOnSamePath(false);
  auto gauge = std::make_unique<Gauge<uint64_t>>(kRelPath, 1);
  ASSERT_TRUE(gauge->IsExposed());
  ASSERT_EQ(gauge->GetAbsPath(), kAbsPath);
  ASSERT_TRUE(ValidateSeries(tvar::TrpcVarGroup::TryGet(kAbsPath, true)));
  auto repetitive_gauge = std::make_unique<Gauge<uint64_t>>(kRelPath, 1);
  ASSERT_FALSE(repetitive_gauge->IsExposed());
  ASSERT_TRUE(repetitive_gauge->GetAbsPath().empty());
  gauge.reset();
  gauge = std::make_unique<Gauge<uint64_t>>(kRelPath, 1);
  ASSERT_TRUE(gauge->IsExposed());
  ASSERT_EQ(gauge->GetAbsPath(), kAbsPath);
  ASSERT_TRUE(ValidateSeries(tvar::TrpcVarGroup::TryGet(kAbsPath, true)));
}

/// @brief Test for Counter not abort on same path.
TEST_F(TestReducer, CounterNotAbortOnSamePath) {
  const std::string kRelPath("user/counter");
  const std::string kAbsPath("/user/counter");
  trpc::tvar::SetWhetherToAbortOnSamePath(false);
  auto counter = std::make_unique<Counter<uint64_t>>(kRelPath, 1);
  ASSERT_TRUE(counter->IsExposed());
  ASSERT_EQ(counter->GetAbsPath(), kAbsPath);
  ASSERT_TRUE(ValidateSeries(tvar::TrpcVarGroup::TryGet(kAbsPath, true)));
  auto repetitive_counter = std::make_unique<Counter<uint64_t>>(kRelPath, 1);
  ASSERT_FALSE(repetitive_counter->IsExposed());
  ASSERT_TRUE(repetitive_counter->GetAbsPath().empty());
  counter.reset();
  counter = std::make_unique<Counter<uint64_t>>(kRelPath, 1);
  ASSERT_TRUE(counter->IsExposed());
  ASSERT_EQ(counter->GetAbsPath(), kAbsPath);
  ASSERT_TRUE(ValidateSeries(tvar::TrpcVarGroup::TryGet(kAbsPath, true)));
}

}  // namespace trpc::testing
