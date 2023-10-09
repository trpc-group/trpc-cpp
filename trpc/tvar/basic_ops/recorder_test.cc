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

#include "trpc/tvar/basic_ops/recorder.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

namespace {

using trpc::tvar::Averager;
using trpc::tvar::AvgBuffer;
using trpc::tvar::CompressedIntAvgTraits;
using trpc::tvar::GetSystemMicroseconds;
using trpc::tvar::IntRecorder;
using trpc::tvar::MergeAvgBuffer;
using trpc::tvar::MinusAvgBuffer;
using trpc::tvar::UpdateAvgBuffer;

}  // namespace

namespace trpc::testing {

/// @brief Test fixture to load config.
class TestRecorder : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    ASSERT_EQ(trpc::TrpcConfig::GetInstance()->Init("trpc/tvar/testing/series.yaml"), 0);
  }

  static void TearDownTestCase() {}
};

/// @brief Test for AvgBuffer.
TEST_F(TestRecorder, TestAvgBuffer) {
  AvgBuffer<int> avg_buffer;
  ASSERT_EQ(avg_buffer.val, 0);
  ASSERT_EQ(avg_buffer.num, 0);
  ASSERT_EQ(avg_buffer.Average(), 0);

  avg_buffer.val = 8;
  avg_buffer.num = 3;
  ASSERT_EQ(avg_buffer.Average(), 2);

  avg_buffer.val = 10;
  avg_buffer.num = 0;
  ASSERT_EQ(avg_buffer.Average(), 0);
}

/// @brief Test for AvgBuffer operator.
TEST_F(TestRecorder, TestOp) {
  // UpdateAvgBuffer
  {
    AvgBuffer<int> buffer;
    UpdateAvgBuffer<int>()(&buffer, 3);
    ASSERT_EQ(buffer.val, 3);
    ASSERT_EQ(buffer.num, 1);
    ASSERT_EQ(buffer.Average(), 3);
    UpdateAvgBuffer<int>()(&buffer, -1);
    ASSERT_EQ(buffer.val, 2);
    ASSERT_EQ(buffer.num, 2);
    ASSERT_EQ(buffer.Average(), 1);
  }

  // MergeAvgBuffer
  {
    AvgBuffer<int> buffer_1;
    buffer_1.num = 1;
    buffer_1.val = 3;
    AvgBuffer<int> buffer_2;
    buffer_2.num = 3;
    buffer_2.val = 1;
    MergeAvgBuffer<int>()(&buffer_1, buffer_2);
    ASSERT_EQ(buffer_1.val, 4);
    ASSERT_EQ(buffer_1.num, 4);
    ASSERT_EQ(buffer_1.Average(), 1);
  }

  // MinusAvgBuffer
  {
    AvgBuffer<int> buffer_1;
    buffer_1.num = 4;
    buffer_1.val = 4;
    AvgBuffer<int> buffer_2;
    buffer_2.num = 1;
    buffer_2.val = 1;
    MinusAvgBuffer<int>()(&buffer_1, buffer_2);
    ASSERT_EQ(buffer_1.val, 3);
    ASSERT_EQ(buffer_1.num, 3);
    ASSERT_EQ(buffer_1.Average(), 1);
  }
}

/// @brief Test for Averager.
TEST_F(TestRecorder, TestAverager) {
  Averager<int> avg;
  {
    auto v = avg.GetValue();
    ASSERT_EQ(v.Average(), 0);
    ASSERT_EQ(v.val, 0);
    ASSERT_EQ(v.num, 0);
  }

  std::thread t1([&avg]() { avg.Update(3); });
  std::thread t2([&avg]() { avg.Update(7); });
  avg.Update(5);
  t1.join();
  t2.join();
  {
    auto v = avg.Reset();
    ASSERT_EQ(v.Average(), 5);
    ASSERT_EQ(v.val, 15);
    ASSERT_EQ(v.num, 3);
  }
  {
    auto v = avg.GetValue();
    ASSERT_EQ(v.Average(), 0);
    ASSERT_EQ(v.val, 0);
    ASSERT_EQ(v.num, 0);
  }

  auto wk_ptr = avg.GetReducerSampler();
  ASSERT_TRUE(wk_ptr.lock());
}

/// @brief Test for CompressedIntAvg GetComplement.
TEST_F(TestRecorder, TestComplement) {
  for (auto&& a : {-10000000, 10000000, 0}) {
    const uint64_t complement = CompressedIntAvgTraits::CompressedIntAvg::GetComplement(a);
    const int64_t b = CompressedIntAvgTraits::CompressedIntAvg::ExtendSignBit(complement);
    ASSERT_EQ(a, b);
  }
}

/// @brief Test for CompressedIntAvg Compress.
TEST_F(TestRecorder, TestCompress) {
  const uint64_t num = 123456;
  const uint64_t sum = 19942906;
  const uint64_t compressed = CompressedIntAvgTraits::CompressedIntAvg::Compress(num, sum);
  ASSERT_EQ(num, CompressedIntAvgTraits::CompressedIntAvg::GetNum(compressed));
  ASSERT_EQ(sum, CompressedIntAvgTraits::CompressedIntAvg::GetSum(compressed));
}

/// @brief Test for compress negative number.
TEST_F(TestRecorder, TestCompressNegtiveNumber) {
  for (auto&& a : {-10000000, 10000000, 0}) {
    const uint64_t sum = CompressedIntAvgTraits::CompressedIntAvg::GetComplement(a);
    const uint64_t num = 123456;
    const uint64_t compressed = CompressedIntAvgTraits::CompressedIntAvg::Compress(num, sum);
    ASSERT_EQ(num, CompressedIntAvgTraits::CompressedIntAvg::GetNum(compressed));
    ASSERT_EQ(a, CompressedIntAvgTraits::CompressedIntAvg::ExtendSignBit(
              CompressedIntAvgTraits::CompressedIntAvg::GetSum(compressed)));
  }
}

/// @brief Test for Recorder negative update.
TEST_F(TestRecorder, TestNegative) {
  Averager<int64_t> recorder;
  IntRecorder recorder_int;

  for (size_t i = 0; i < 3; ++i) {
    recorder.Update(-2);
    recorder_int.Update(-2);
  }

  auto ret = recorder.GetValue();
  auto ret_int = recorder_int.GetValue();

  ASSERT_EQ(3, ret.num);
  ASSERT_EQ(-2, ret.Average());
  ASSERT_EQ(3, ret_int.num);
  ASSERT_EQ(-2, ret_int.Average());
}

/// @brief Test for Recorder positive overflow.
TEST_F(TestRecorder, TestPositiveOverflow) {
  IntRecorder recorder_int;

  for (int i = 0; i < 5; ++i) {
    recorder_int.Update(std::numeric_limits<int64_t>::max());
  }

  auto ret_int = recorder_int.GetValue();
  ASSERT_EQ(5, ret_int.num);
  ASSERT_EQ(std::numeric_limits<int>::max(), ret_int.Average());
}

/// @brief Test for Recorder negative overflow.
TEST_F(TestRecorder, TestNegtiveOverflow) {
  IntRecorder recorder_int;

  for (int i = 0; i < 5; ++i) {
    recorder_int.Update(std::numeric_limits<int64_t>::min());
  }

  auto ret_int = recorder_int.GetValue();
  ASSERT_EQ(5, ret_int.num);
  ASSERT_EQ(std::numeric_limits<int>::min(), ret_int.Average());
}

namespace {

constexpr int kThreadsNum = 4;
constexpr int kOpsPerThread = 5000;

template <typename T>
auto RecorderPerf() {
  T recorder;
  std::atomic_uint64_t total_time{0};
  std::vector<std::thread> vec;
  vec.reserve(kThreadsNum);
  for (int i = 0; i < kThreadsNum; ++i) {
    vec.emplace_back([&recorder, &total_time]() {
      auto begin_ts = GetSystemMicroseconds();
      for (int i = 0; i < kOpsPerThread; ++i) {
        recorder.Update(i);
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
  std::cout << "recorder takes " << total_time / kThreadsNum << "us per " << kOpsPerThread
            << " samples with " << kThreadsNum << " threads" << std::endl;
  return recorder.GetValue();
}

}  // namespace

/// @brief Compare IntRecorder to traditional atomic.
TEST_F(TestRecorder, IntRecorderPerf) {
  auto ret = RecorderPerf<IntRecorder>();
  ASSERT_EQ(kThreadsNum * kOpsPerThread, ret.num);
  ASSERT_EQ((static_cast<int64_t>(kOpsPerThread) - 1) / 2, ret.Average());
}

/// @brief Compare Averager to traditional atomic.
TEST_F(TestRecorder, AveragerPerf) {
  auto ret = RecorderPerf<Averager<int64_t>>();
  ASSERT_EQ(kThreadsNum * kOpsPerThread, ret.num);
  ASSERT_EQ((static_cast<int64_t>(kOpsPerThread) - 1) / 2, ret.Average());
}

/// @brief Test Recorder exposed.
TEST_F(TestRecorder, Exposed) {
  std::weak_ptr<Averager<int>::ReducerSamplerType> averager_reducer_ptr;
  std::weak_ptr<IntRecorder::ReducerSamplerType> int_recorder_reducer_ptr;

  {
    std::weak_ptr<Averager<int>::SeriesSamplerType> averager_series_ptr;
    std::weak_ptr<IntRecorder::SeriesSamplerType> int_recorder_series_ptr;
    Averager<int> averager("user/averager");
    ASSERT_EQ(averager.GetAbsPath(), "/user/averager");
    averager_series_ptr = averager.GetSeriesSampler();
    // Series sampler will not open as ResultType is AvgBuffer.
    ASSERT_FALSE(averager_series_ptr.lock());
    averager_reducer_ptr = averager.GetReducerSampler();
    // Reducer sampler will open anyway.
    ASSERT_TRUE(averager_reducer_ptr.lock());

    IntRecorder int_recorder("user/int_recorder");
    ASSERT_EQ(int_recorder.GetAbsPath(), "/user/int_recorder");
    int_recorder_series_ptr = int_recorder.GetSeriesSampler();
    // Series sampler will not open as ResultType is AvgBuffer.
    ASSERT_FALSE(int_recorder_series_ptr.lock());
    int_recorder_reducer_ptr = int_recorder.GetReducerSampler();
    // Reducer sampler will open anyway.
    ASSERT_TRUE(int_recorder_reducer_ptr.lock());
  }

  std::this_thread::sleep_for(std::chrono::seconds(3));
  ASSERT_FALSE(averager_reducer_ptr.lock());
  ASSERT_FALSE(int_recorder_reducer_ptr.lock());
}

/// @brief Test for Average not abort on same path.
TEST_F(TestRecorder, AveragerNotAbortOnSamePath) {
  const std::string kRelPath("user/averager");
  const std::string kAbsPath("/user/averager");
  trpc::tvar::SetWhetherToAbortOnSamePath(false);
  auto averager = std::make_unique<Averager<int>>(kRelPath);
  ASSERT_TRUE(averager->IsExposed());
  ASSERT_EQ(averager->GetAbsPath(), kAbsPath);

  auto repetitive_averager = std::make_unique<Averager<int>>(kRelPath);
  ASSERT_FALSE(repetitive_averager->IsExposed());
  ASSERT_TRUE(repetitive_averager->GetAbsPath().empty());

  averager.reset();
  averager = std::make_unique<Averager<int>>(kRelPath);
  ASSERT_TRUE(averager->IsExposed());
  ASSERT_EQ(averager->GetAbsPath(), kAbsPath);
}

/// @brief Test for IntRecorder not abort on same path.
TEST_F(TestRecorder, IntRecorderNotAbortOnSamePath) {
  const std::string kRelPath("user/int_recorder");
  const std::string kAbsPath("/user/int_recorder");
  trpc::tvar::SetWhetherToAbortOnSamePath(false);
  auto int_recorder = std::make_unique<IntRecorder>(kRelPath);
  ASSERT_TRUE(int_recorder->IsExposed());
  ASSERT_EQ(int_recorder->GetAbsPath(), kAbsPath);

  auto repetitive_int_recorder = std::make_unique<IntRecorder>(kRelPath);
  ASSERT_FALSE(repetitive_int_recorder->IsExposed());
  ASSERT_TRUE(repetitive_int_recorder->GetAbsPath().empty());

  int_recorder.reset();
  int_recorder = std::make_unique<IntRecorder>(kRelPath);
  ASSERT_TRUE(int_recorder->IsExposed());
  ASSERT_EQ(int_recorder->GetAbsPath(), kAbsPath);
}

}  // namespace trpc::testing
