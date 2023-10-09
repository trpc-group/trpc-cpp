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

#include "trpc/tvar/basic_ops/status.h"

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#include "gtest/gtest.h"

#include "trpc/tvar/common/tvar_group.h"

namespace {

using trpc::tvar::Status;

/// @brief Validate history data format.
bool ValidateSeries(std::optional<Json::Value> gauge_value) {
  return gauge_value.has_value() && gauge_value->isObject() && gauge_value->isMember("now") &&
         gauge_value->isMember("latest_sec") && gauge_value->isMember("latest_min") &&
         gauge_value->isMember("latest_hour") && gauge_value->isMember("latest_day");
}

}  // namespace

namespace trpc::testing {

/// @brief Test fixture to load config.
class TestStatus : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    ASSERT_EQ(trpc::TrpcConfig::GetInstance()->Init("trpc/tvar/testing/series.yaml"), 0);
  }

  static void TearDownTestCase() {}
};

/// @brief Test for not-exposed Status.
TEST_F(TestStatus, NotExposed) {
  Status<int> st(9);
  ASSERT_EQ(9, st.GetValue());
  st.SetValue(10);
  ASSERT_EQ(10, st.GetValue());
  st.Update(11);
  ASSERT_EQ(11, st.GetValue());
  // Series sampler is not opened in not-exposed situation.
  ASSERT_TRUE(st.GetSeriesValue().empty());

  Status<double> st2;
  ASSERT_EQ(0.0, st2.GetValue());
  st2.SetValue(10.0);
  ASSERT_EQ(10.0, st2.GetValue());
}

/// @brief Test for exposed Status.
TEST_F(TestStatus, Exposed) {
  std::weak_ptr<Status<int>::SeriesSamplerType> series_sampler_ptr;

  {
    Status<int> st("status", 9);
    ASSERT_EQ(st.GetAbsPath(), "/status");

    series_sampler_ptr = st.GetSeriesSampler();
    // Series sampler is open in exposed situation.
    ASSERT_TRUE(series_sampler_ptr.lock());
    ASSERT_TRUE(ValidateSeries(tvar::TrpcVarGroup::TryGet("/status", true)));
  }

  // Wait for sampler collector to release shared ptr.
  std::this_thread::sleep_for(std::chrono::seconds(3));
  // Yes, fully destroyed.
  ASSERT_FALSE(series_sampler_ptr.lock());
}

/// @brief Test for not abort on same path.
TEST_F(TestStatus, NotAbortOnSamePath) {
  const std::string kRelPath("status");
  const std::string kAbsPath("/status");
  // Close abort.
  trpc::tvar::SetWhetherToAbortOnSamePath(false);
  auto st = std::make_unique<Status<int>>(kRelPath, 9);
  ASSERT_TRUE(st->IsExposed());
  ASSERT_EQ(st->GetAbsPath(), kAbsPath);
  ASSERT_TRUE(ValidateSeries(tvar::TrpcVarGroup::TryGet(kAbsPath, true)));

  // Repeat Status open same path.
  auto repetitive_st = std::make_unique<Status<int>>(kRelPath, 9);
  ASSERT_FALSE(repetitive_st->IsExposed());
  ASSERT_TRUE(repetitive_st->GetAbsPath().empty());

  // Release first one.
  st.reset();
  // Able to create a new one.
  st = std::make_unique<Status<int>>(kRelPath, 9);
  ASSERT_TRUE(st->IsExposed());
  ASSERT_EQ(st->GetAbsPath(), kAbsPath);
  ASSERT_TRUE(ValidateSeries(tvar::TrpcVarGroup::TryGet(kAbsPath, true)));
}

}  // namespace trpc::testing
