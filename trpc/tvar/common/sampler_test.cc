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

#include "trpc/tvar/common/sampler.h"

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

namespace {

/// @brief Simple sampler to record how many times been taken sample.
class DebugSampler : public trpc::tvar::Sampler {
 public:
  DebugSampler() : n_called_(0) {}

  ~DebugSampler() { ++n_destroy_; }

  void TakeSample() { ++n_called_; }

  int called_count() const { return n_called_; }

  int n_called_;

  static std::atomic_int n_destroy_;
};

std::atomic_int DebugSampler::n_destroy_ = 0;

}  // namespace

namespace trpc::testing {

/// @brief Test in single thread.
TEST(SamplerTest, TestSingleThreaded) {
  constexpr int N = 100;
  std::vector<std::shared_ptr<DebugSampler>> s;
  s.reserve(N);
  for (int i = 0; i < N; ++i) {
    s.emplace_back(std::make_shared<DebugSampler>());
    s[i]->Schedule();
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));
  for (int i = 0; i < N; ++i) {
    ASSERT_LE(1, s[i]->called_count());
  }
  ASSERT_EQ(0, DebugSampler::n_destroy_);

  for (int i = 0; i < N; ++i) {
    // Notify sampler_collector.
    s[i]->Destroy();
    // Release shared ptr by ourself.
    s[i].reset();
  }

  // Let sampler_collector take time to release shared ptr.
  std::this_thread::sleep_for(std::chrono::seconds(2));
  ASSERT_EQ(N, DebugSampler::n_destroy_);
}

/// @brief Test in multiple threads.
TEST(SamplerTest, TestMultiThreaded) {
  auto n_destroy_back = DebugSampler::n_destroy_.load(std::memory_order_relaxed);

  std::vector<std::thread> vec;
  vec.reserve(10);
  for (int i = 0; i < 10; ++i) {
    vec.emplace_back([]() {
      constexpr int N = 100;
      std::vector<std::shared_ptr<DebugSampler>> s;
      s.reserve(N);
      for (int i = 0; i < N; ++i) {
        s.emplace_back(std::make_shared<DebugSampler>());
        s[i]->Schedule();
      }
      std::this_thread::sleep_for(std::chrono::seconds(2));
      for (int i = 0; i < N; ++i) {
        ASSERT_LE(1, s[i]->called_count());
      }
      for (int i = 0; i < N; ++i) {
        s[i]->Destroy();
        s[i].reset();
      }
    });
  }
  for (auto& v : vec) {
    v.join();
  }
  std::this_thread::sleep_for(std::chrono::seconds(2));
  ASSERT_EQ(n_destroy_back + 100 * vec.size(),
            static_cast<size_t>(DebugSampler::n_destroy_.load(std::memory_order_relaxed)));
}

/// @brief Test for sampler_collector thread.
TEST(SamplerTest, SamplerCollectorThread) {
  trpc::tvar::SamplerCollectorThreadStart();
  trpc::tvar::SamplerCollectorThreadStop();
  constexpr int N = 100;
  std::vector<std::shared_ptr<DebugSampler>> s;
  s.reserve(N);
  for (int i = 0; i < N; ++i) {
    s.emplace_back(std::make_shared<DebugSampler>());
    s[i]->Schedule();
  }
  // No sampler_collector to take sample now.
  std::this_thread::sleep_for(std::chrono::seconds(2));
  for (int i = 0; i < N; ++i) {
    ASSERT_LE(0, s[i]->called_count());
  }
  ASSERT_NE(0, DebugSampler::n_destroy_);
  for (int i = 0; i < N; ++i) {
    s[i]->Destroy();
    s[i].reset();
  }
  std::this_thread::sleep_for(std::chrono::seconds(2));
  ASSERT_NE(N, DebugSampler::n_destroy_);
  trpc::tvar::SamplerCollectorThreadStop();
  trpc::tvar::SamplerCollectorThreadStart();
}

}  // namespace trpc::testing
