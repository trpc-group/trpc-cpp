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

#include "trpc/tvar/basic_ops/passive_status.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#include "gtest/gtest.h"

namespace {

using trpc::tvar::PassiveStatus;

/// @brief Validate history data format.
bool ValidateSeries(std::optional<Json::Value> gauge_value) {
  return gauge_value.has_value() && gauge_value->isObject() && gauge_value->isMember("now") &&
         gauge_value->isMember("latest_sec") && gauge_value->isMember("latest_min") &&
         gauge_value->isMember("latest_hour") && gauge_value->isMember("latest_day");
}

}  // namespace

namespace trpc::testing {

/// @brief Test fixture to load config.
class TestPassiveStatus : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    ASSERT_EQ(trpc::TrpcConfig::GetInstance()->Init("trpc/tvar/testing/series.yaml"), 0);
  }

  static void TearDownTestCase() {}
};

/// @brief Test for not-exposed PassiveStatus.
TEST_F(TestPassiveStatus, NotExposed) {
  std::weak_ptr<PassiveStatus<int>::ReducerSamplerType> reducer_sampler_ptr;

  {
    std::atomic_int32_t ret{0};
    // Define a not-exposed PassiveStatus tvar.
    PassiveStatus<int> st([&ret]() { return ret.load(std::memory_order_relaxed); });
    ASSERT_EQ(0, st.GetValue());
    ret.store(10, std::memory_order_relaxed);
    ASSERT_EQ(10, st.GetValue());

    reducer_sampler_ptr = st.GetReducerSampler();
    // Reducer sampler is created if not found.
    ASSERT_TRUE(reducer_sampler_ptr.lock());
    // Series sampler will not be opened in not-exposed situation.
    ASSERT_TRUE(st.GetSeriesValue().empty());
  }

  // Let sampler collector take time to release reducer sampler shared ptr.
  std::this_thread::sleep_for(std::chrono::seconds(3));
  // Yes, reducer sampler is destructed now.
  ASSERT_FALSE(reducer_sampler_ptr.lock());
}

/// @brief Test for exposed PassiveStatus.
TEST_F(TestPassiveStatus, Exposed) {
  std::weak_ptr<PassiveStatus<int>::SeriesSamplerType> series_sampler_ptr;
  std::weak_ptr<PassiveStatus<int>::ReducerSamplerType> reducer_sampler_ptr;

  {
    std::atomic_int32_t ret{0};
    // Define a exposed PassiveStatus tvar.
    PassiveStatus<int> st("passive_status",
                          [&ret]() { return ret.load(std::memory_order_relaxed); });
    ASSERT_EQ(st.GetAbsPath(), "/passive_status");

    series_sampler_ptr = st.GetSeriesSampler();
    // Yes, exposed PassiveStatus will try to open series sampler, with T and config allowed.
    ASSERT_TRUE(series_sampler_ptr.lock());
    // To imitate admin request.
    ASSERT_TRUE(ValidateSeries(tvar::TrpcVarGroup::TryGet("/passive_status", true)));
    reducer_sampler_ptr = st.GetReducerSampler();
    // Reducer sampler is created if not found.
    ASSERT_TRUE(reducer_sampler_ptr.lock());
  }

  // Let sampler collector take time to release shared ptr.
  std::this_thread::sleep_for(std::chrono::seconds(3));
  ASSERT_FALSE(reducer_sampler_ptr.lock());
  ASSERT_FALSE(series_sampler_ptr.lock());
}

/// @brief Test for not abort on same path.
TEST_F(TestPassiveStatus, NotAbortOnSamePath) {
  const std::string kRelPath("passive_status");
  const std::string kAbsPath("/passive_status");
  std::atomic_int32_t ret{0};
  // Close abort.
  trpc::tvar::SetWhetherToAbortOnSamePath(false);
  // Exposed tvar.
  auto st = std::make_unique<PassiveStatus<int>>(
      kRelPath, [&ret]() { return ret.load(std::memory_order_relaxed); });
  ASSERT_TRUE(st->IsExposed());
  ASSERT_EQ(st->GetAbsPath(), kAbsPath);
  ASSERT_TRUE(ValidateSeries(tvar::TrpcVarGroup::TryGet(kAbsPath, true)));

  // Try to define a new tvar with same path as previous.
  auto repetitive_st = std::make_unique<PassiveStatus<int>>(
      kRelPath, [&ret]() { return ret.load(std::memory_order_relaxed); });
  // The repeat one fails to register to tvar group.
  ASSERT_FALSE(repetitive_st->IsExposed());
  ASSERT_TRUE(repetitive_st->GetAbsPath().empty());
  // Destroy previous one.
  st.reset();

  // Now we are able to reuse the path to create a new one.
  st = std::make_unique<PassiveStatus<int>>(kRelPath, [&ret]() { return ret.load(std::memory_order_relaxed); });
  ASSERT_TRUE(st->IsExposed());
  ASSERT_EQ(st->GetAbsPath(), kAbsPath);
  ASSERT_TRUE(ValidateSeries(tvar::TrpcVarGroup::TryGet(kAbsPath, true)));
}

}  // namespace trpc::testing
