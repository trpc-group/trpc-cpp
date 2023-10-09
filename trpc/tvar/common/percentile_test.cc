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

#include "trpc/tvar/common/percentile.h"

#include <iostream>
#include <memory>
#include <vector>

#include "gtest/gtest.h"

namespace {

using trpc::tvar::GlobalPercentileSamples;
using trpc::tvar::PercentileInterval;
using trpc::tvar::PercentileSamples;
using trpc::tvar::WriteMostlyPercentile;

}  // namespace

namespace trpc::testing {

/// @brief Test for add operation.
TEST(PercentileTest, Add) {
  WriteMostlyPercentile p;
  for (int j = 0; j < 10; ++j) {
    for (int i = 0; i < 10000; ++i) {
      p.Update(i + 1);
    }
    auto b = p.Reset();
    uint32_t last_value = 0;
    for (uint32_t k = 1; k <= 10u; ++k) {
      uint32_t value = b.GetNumber(k / 10.0);
      ASSERT_GE(value, last_value);
      last_value = value;
      ASSERT_GT(value, (k * 1000 - 500));
      std::cout << "k=" << k << " value=" << value << std::endl;
      ASSERT_LT(value, (k * 1000 + 500));
    }
    std::cout << "99%:" << b.GetNumber(0.99) << ' ' << "99.9%:" << b.GetNumber(0.999) << ' '
              << "99.99%:" << b.GetNumber(0.9999) << std::endl;
  }
}

/// @brief Merge 2 PercentileIntervals b1 and b2. b2 has double SAMPLE_SIZE
///        and num_added. Remaining samples of b1 and b2 in merged result should
///        be 1:2 approximately.
TEST(PercentileTest, merge1) {
  constexpr size_t N = 1000;
  constexpr size_t SAMPLE_SIZE = 32;
  size_t belong_to_b1 = 0;
  size_t belong_to_b2 = 0;

  for (int repeat = 0; repeat < 100; ++repeat) {
    PercentileInterval<SAMPLE_SIZE * 3> b0;
    PercentileInterval<SAMPLE_SIZE> b1;
    for (size_t i = 0; i < N; ++i) {
      if (b1.Full()) {
        b0.Merge(b1);
        b1.Clear();
      }
      ASSERT_TRUE(b1.Add32(i));
    }
    b0.Merge(b1);
    PercentileInterval<SAMPLE_SIZE * 2> b2;
    for (size_t i = 0; i < N * 2; ++i) {
      if (b2.Full()) {
        b0.Merge(b2);
        b2.Clear();
      }
      ASSERT_TRUE(b2.Add32(i + N));
    }
    b0.Merge(b2);
    for (size_t i = 0; i < b0.SampleCount(); ++i) {
      if (b0.GetSampleAt(i) < N) {
        ++belong_to_b1;
      } else {
        ++belong_to_b2;
      }
    }
  }
  ASSERT_LT(fabs(belong_to_b1 / static_cast<double>(belong_to_b2) - 0.5), 0.2);
  std::cout << "belong_to_b1=" << belong_to_b1 << " belong_to_b2=" << belong_to_b2 << std::endl;
}

/// @brief Merge 2 PercentileIntervals b1 and b2 with same SAMPLE_SIZE. Add N1
///        samples to b1 and add N2 samples to b2, Remaining samples of b1 and
///        b2 in merged result should be N1:N2 approximately.
TEST(PercentileTest, merge2) {
  constexpr size_t N1 = 1000;
  constexpr size_t N2 = 400;
  size_t belong_to_b1 = 0;
  size_t belong_to_b2 = 0;

  for (int repeat = 0; repeat < 100; ++repeat) {
    PercentileInterval<64> b0;
    PercentileInterval<64> b1;
    for (size_t i = 0; i < N1; ++i) {
      if (b1.Full()) {
        b0.Merge(b1);
        b1.Clear();
      }
      ASSERT_TRUE(b1.Add32(i));
    }
    b0.Merge(b1);
    PercentileInterval<64> b2;
    for (size_t i = 0; i < N2; ++i) {
      if (b2.Full()) {
        b0.Merge(b2);
        b2.Clear();
      }
      ASSERT_TRUE(b2.Add32(i + N1));
    }
    b0.Merge(b2);
    for (size_t i = 0; i < b0.SampleCount(); ++i) {
      if (b0.GetSampleAt(i) < N1) {
        ++belong_to_b1;
      } else {
        ++belong_to_b2;
      }
    }
  }
  ASSERT_LT(fabs(belong_to_b1 / static_cast<double>(belong_to_b2) - N1 / static_cast<double>(N2)),
            0.2);
  std::cout << "belong_to_b1=" << belong_to_b1 << " belong_to_b2=" << belong_to_b2 << std::endl;
}

/// @brief Test for CombineOf.
TEST(PercentileTest, CombineOf) {
  // Combine multiple percentle samplers into one.
  constexpr int num_samplers = 10;
  // Define a base time to make all samples are in the same interval.
  constexpr uint32_t base = (1 << 30) + 1;

  const int N = 1000;
  size_t belongs[num_samplers] = {0};
  size_t total = 0;
  for (int repeat = 0; repeat < 100; ++repeat) {
    std::vector<std::unique_ptr<WriteMostlyPercentile>> p;
    p.reserve(num_samplers);
    for (int i = 0; i < num_samplers; ++i) {
      p.emplace_back(std::make_unique<WriteMostlyPercentile>());
      for (int j = 0; j < N * (i + 1); ++j) {
        p[i]->Update(base + i * (i + 1) * N / 2 + j);
      }
    }
    std::vector<GlobalPercentileSamples> result;
    result.reserve(num_samplers);
    for (auto& i : p) {
      result.push_back(i->GetValue());
    }
    PercentileSamples<510> g;
    g.CombineOf(result.begin(), result.end());
    for (size_t i = 0; i < trpc::tvar::NUM_INTERVALS; ++i) {
      auto intervals = g.GetIntervals();
      if (intervals[i] == nullptr) {
        continue;
      }
      auto& temp = *intervals[i];
      total += temp.SampleCount();
      for (size_t j = 0; j < temp.SampleCount(); ++j) {
        for (int k = 0; k < num_samplers; ++k) {
          if ((temp.GetSampleAt(j) - base) / N < (k + 1) * (k + 2) / 2u) {
            belongs[k]++;
            break;
          }
        }
      }
    }
  }
  for (int i = 0; i < num_samplers; ++i) {
    double expect_ratio = static_cast<double>(i + 1) * 2 / (num_samplers * (num_samplers + 1));
    double actual_ratio = static_cast<double>(belongs[i]) / total;
    ASSERT_LT(fabs(expect_ratio / actual_ratio) - 1, 0.2);
    std::cout << "expect_ratio=" << expect_ratio << " actual_ratio=" << actual_ratio << std::endl;
  }
}

}  // namespace trpc::testing
