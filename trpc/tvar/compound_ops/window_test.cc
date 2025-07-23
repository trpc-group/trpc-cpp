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

#include "trpc/tvar/compound_ops/window.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "gtest/gtest.h"

#include "trpc/tvar/basic_ops/passive_status.h"
#include "trpc/tvar/basic_ops/recorder.h"
#include "trpc/tvar/basic_ops/reducer.h"
#include "trpc/tvar/common/op_util.h"

namespace {

using trpc::tvar::Averager;
using trpc::tvar::Counter;
using trpc::tvar::Gauge;
using trpc::tvar::GetSystemMicroseconds;
using trpc::tvar::IntRecorder;
using trpc::tvar::Maxer;
using trpc::tvar::Miner;
using trpc::tvar::PassiveStatus;
using trpc::tvar::PerSecond;
using trpc::tvar::Window;

}  // namespace

namespace trpc::testing {

/// @brief Test fixture to load config.
class TestWindow : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    ASSERT_EQ(trpc::TrpcConfig::GetInstance()->Init("trpc/tvar/testing/series.yaml"), 0);
  }

  static void TearDownTestCase() {}
};

/// @brief Test sampler window size change.
TEST_F(TestWindow, DefaultWindowSize) {
  Counter<int64_t> c;
  Window<Counter<int64_t>> w(&c);
  auto sampler = c.GetReducerSampler().lock();
  ASSERT_TRUE(sampler);
  // Default window size is 10.
  ASSERT_EQ(w.WindowSize(),
            trpc::TrpcConfig::GetInstance()->GetGlobalConfig().tvar_config.window_size);
  ASSERT_EQ(sampler->GetWindowSize(),
            trpc::TrpcConfig::GetInstance()->GetGlobalConfig().tvar_config.window_size);
  std::cout << "WindowSize=" << w.WindowSize() << std::endl;

  Window<Counter<int64_t>> w1(&c, 1);
  ASSERT_EQ(w1.WindowSize(), 1);
  // Window size is not allowed to decrease.
  ASSERT_EQ(sampler->GetWindowSize(),
            trpc::TrpcConfig::GetInstance()->GetGlobalConfig().tvar_config.window_size);

  Window<Counter<int64_t>> w11(&c, 110);
  ASSERT_EQ(w11.WindowSize(), 110);
  // Window size is allowed to increase.
  ASSERT_EQ(sampler->GetWindowSize(), 110);
}

/// @brief Test for Window.
TEST_F(TestWindow, TestWindow) {
  Counter<int64_t> c1;
  Gauge<int64_t> c2;
  Maxer<int64_t> c3;
  Miner<int64_t> c4;
  Averager<int64_t> c5;
  IntRecorder c6;
  PassiveStatus<uint64_t> c7([begin = GetSystemMicroseconds()]() {
    if (auto end = GetSystemMicroseconds(); end > begin) {
      return end - begin;
    }
    return static_cast<uint64_t>(0);
  });

  Window<Counter<int64_t>> w11(&c1, 1);
  ASSERT_EQ(w11.WindowSize(), 1);
  Window<Counter<int64_t>> w12(&c1, 2);
  ASSERT_EQ(w12.WindowSize(), 2);
  Window<Counter<int64_t>> w13(&c1, 3);
  ASSERT_EQ(w13.WindowSize(), 3);

  Window<Gauge<int64_t>> w21(&c2, 1);
  ASSERT_EQ(w21.WindowSize(), 1);
  Window<Gauge<int64_t>> w22(&c2, 2);
  ASSERT_EQ(w22.WindowSize(), 2);
  Window<Gauge<int64_t>> w23(&c2, 3);
  ASSERT_EQ(w23.WindowSize(), 3);

  Window<Maxer<int64_t>> w31(&c3, 1);
  Window<Maxer<int64_t>> w32(&c3, 2);
  Window<Maxer<int64_t>> w33(&c3, 3);

  Window<Miner<int64_t>> w41(&c4, 1);
  Window<Miner<int64_t>> w42(&c4, 2);
  Window<Miner<int64_t>> w43(&c4, 3);

  Window<Averager<int64_t>> w51(&c5, 1);
  Window<Averager<int64_t>> w52(&c5, 2);
  Window<Averager<int64_t>> w53(&c5, 3);

  Window<IntRecorder> w61(&c6, 1);
  Window<IntRecorder> w62(&c6, 2);
  Window<IntRecorder> w63(&c6, 3);

  Window<PassiveStatus<uint64_t>> w71(&c7, 1);
  Window<PassiveStatus<uint64_t>> w72(&c7, 2);
  Window<PassiveStatus<uint64_t>> w73(&c7, 3);

  PerSecond<Counter<int64_t>> p11(&c1, 1);
  PerSecond<Counter<int64_t>> p12(&c1, 2);
  PerSecond<Counter<int64_t>> p13(&c1, 3);

  PerSecond<Gauge<int64_t>> p21(&c2, 1);
  PerSecond<Gauge<int64_t>> p22(&c2, 2);
  PerSecond<Gauge<int64_t>> p23(&c2, 3);

  PerSecond<Maxer<int64_t>> p31(&c3, 1);
  PerSecond<Maxer<int64_t>> p32(&c3, 2);
  PerSecond<Maxer<int64_t>> p33(&c3, 3);

  PerSecond<Miner<int64_t>> p41(&c4, 1);
  PerSecond<Miner<int64_t>> p42(&c4, 2);
  PerSecond<Miner<int64_t>> p43(&c4, 3);

  PerSecond<PassiveStatus<uint64_t>> p71(&c7, 1);
  PerSecond<PassiveStatus<uint64_t>> p72(&c7, 2);
  PerSecond<PassiveStatus<uint64_t>> p73(&c7, 3);

  constexpr int N = 6000;
  int count = 0;
  int total_count = 0;
  auto last_time = GetSystemMicroseconds();
  for (int i = 1; i <= N; ++i) {
    c1.Add(1);
    c2.Add(1);
    c3.Update(N - i);
    c4.Update(i);
    c5.Update(i);
    c6.Update(i);
    ++count;
    ++total_count;
    int64_t now = GetSystemMicroseconds();
    if (now - last_time >= 1000000L) {
      last_time = now;
      ASSERT_EQ(total_count, c1.GetValue());
      ASSERT_EQ(total_count, c2.GetValue());
      ASSERT_EQ(total_count, c5.GetValue().num);
      ASSERT_EQ(total_count, c6.GetValue().num);
      std::cout << "c1=" << total_count << " count=" << count << " w11=" << w11.GetValue()
                << " w12=" << w12.GetValue() << " w13=" << w13.GetValue()
                << " w21=" << w21.GetValue() << " w22=" << w22.GetValue()
                << " w23=" << w23.GetValue() << " w31=" << w31.GetValue()
                << " w32=" << w32.GetValue() << " w33=" << w33.GetValue()
                << " w41=" << w41.GetValue() << " w42=" << w42.GetValue()
                << " w43=" << w43.GetValue() << " w51=" << w51.GetValue().Average()
                << " w52=" << w52.GetValue().Average() << " w53=" << w53.GetValue().Average()
                << " w61=" << w61.GetValue().Average() << " w62=" << w62.GetValue().Average()
                << " w63=" << w63.GetValue().Average() << " w71=" << w71.GetValue()
                << " w72=" << w72.GetValue() << " w73=" << w73.GetValue() << std::endl;
      std::cout << "c1=" << total_count << " count=" << count << " p11=" << p11.GetValue()
                << " p12=" << p12.GetValue() << " p13=" << p13.GetValue()
                << " p21=" << p21.GetValue() << " p22=" << p22.GetValue()
                << " p23=" << p23.GetValue() << " p31=" << p31.GetValue()
                << " p32=" << p32.GetValue() << " p33=" << p33.GetValue()
                << " p41=" << p41.GetValue() << " p42=" << p42.GetValue()
                << " p43=" << p43.GetValue() << " p71=" << p71.GetValue()
                << " p72=" << p72.GetValue() << " p73=" << p73.GetValue() << std::endl;
      count = 0;
    } else {
      std::this_thread::sleep_for(std::chrono::microseconds(950));
    }
  }

  ASSERT_TRUE(c1.GetReducerSampler().lock());
  ASSERT_TRUE(c2.GetReducerSampler().lock());
  ASSERT_TRUE(c3.GetReducerSampler().lock());
  ASSERT_TRUE(c4.GetReducerSampler().lock());
  ASSERT_TRUE(c5.GetReducerSampler().lock());
  ASSERT_TRUE(c6.GetReducerSampler().lock());
}

/// @brief Test for window exposed.
TEST_F(TestWindow, Exposed) {
  Counter<int64_t> c1;
  Gauge<int64_t> c2;
  Maxer<int64_t> c3;
  Miner<int64_t> c4;
  Averager<int64_t> c5;
  IntRecorder c6;
  PassiveStatus<uint64_t> c7([begin = GetSystemMicroseconds()]() {
    if (auto end = GetSystemMicroseconds(); end > begin) {
      return end - begin;
    }
    return static_cast<uint64_t>(0);
  });

  std::weak_ptr<Window<Counter<int64_t>>::SeriesSamplerType> w11_ptr;
  std::weak_ptr<Window<Gauge<int64_t>>::SeriesSamplerType> w21_ptr;
  std::weak_ptr<Window<Maxer<int64_t>>::SeriesSamplerType> w31_ptr;
  std::weak_ptr<Window<Miner<int64_t>>::SeriesSamplerType> w41_ptr;
  std::weak_ptr<Window<Averager<int64_t>>::SeriesSamplerType> w51_ptr;
  std::weak_ptr<Window<IntRecorder>::SeriesSamplerType> w61_ptr;
  std::weak_ptr<Window<PassiveStatus<uint64_t>>::SeriesSamplerType> w71_ptr;

  auto parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/a/b");
  {
    Window<Counter<int64_t>> w11("w11", &c1);
    ASSERT_EQ(w11.GetAbsPath(), "/w11");
    auto value11 = tvar::TrpcVarGroup::TryGet("/w11", true);
    ASSERT_TRUE(value11->isObject());
    ASSERT_TRUE(value11->isMember("now"));
    w11_ptr = w11.GetSeries();
    ASSERT_TRUE(w11_ptr.lock());
    Window<Counter<int64_t>> w12(parent, "w12", &c1);
    ASSERT_EQ(w12.GetAbsPath(), "/a/b/w12");
    auto value12 = tvar::TrpcVarGroup::TryGet("/a/b/w12", true);
    ASSERT_TRUE(value12->isObject());
    ASSERT_TRUE(value12->isMember("now"));
    Window<Gauge<int64_t>> w21("w21", &c2);
    auto value21 = tvar::TrpcVarGroup::TryGet("/w21", true);
    ASSERT_TRUE(value21->isObject());
    ASSERT_TRUE(value21->isMember("now"));
    w21_ptr = w21.GetSeries();
    ASSERT_TRUE(w21_ptr.lock());
    Window<Gauge<int64_t>> w22(parent, "w22", &c2);
    auto value22 = tvar::TrpcVarGroup::TryGet("/a/b/w22", true);
    ASSERT_TRUE(value22->isObject());
    ASSERT_TRUE(value22->isMember("now"));
    Window<Maxer<int64_t>> w31("w31", &c3);
    auto value31 = tvar::TrpcVarGroup::TryGet("/w31", true);
    ASSERT_TRUE(value31->isObject());
    ASSERT_TRUE(value31->isMember("now"));
    w31_ptr = w31.GetSeries();
    ASSERT_TRUE(w31_ptr.lock());
    Window<Maxer<int64_t>> w32(parent, "w32", &c3);
    auto value32 = tvar::TrpcVarGroup::TryGet("/a/b/w32", true);
    ASSERT_TRUE(value32->isObject());
    ASSERT_TRUE(value32->isMember("now"));
    Window<Miner<int64_t>> w41("w41", &c4);
    auto value41 = tvar::TrpcVarGroup::TryGet("/w41", true);
    ASSERT_TRUE(value41->isObject());
    ASSERT_TRUE(value41->isMember("now"));
    w41_ptr = w41.GetSeries();
    ASSERT_TRUE(w41_ptr.lock());
    Window<Miner<int64_t>> w42(parent, "w42", &c4);
    auto value42 = tvar::TrpcVarGroup::TryGet("/a/b/w42", true);
    ASSERT_TRUE(value42->isObject());
    ASSERT_TRUE(value42->isMember("now"));
    Window<Averager<int64_t>> w51("w51", &c5);
    auto value51 = tvar::TrpcVarGroup::TryGet("/w51", true);
    ASSERT_TRUE(value51->isObject());
    ASSERT_TRUE(value51->isMember("now"));
    w51_ptr = w51.GetSeries();
    ASSERT_TRUE(w51_ptr.lock());
    Window<Averager<int64_t>> w52(parent, "w52", &c5);
    auto value52 = tvar::TrpcVarGroup::TryGet("/a/b/w52", true);
    ASSERT_TRUE(value52->isObject());
    ASSERT_TRUE(value52->isMember("now"));
    Window<IntRecorder> w61("w61", &c6);
    auto value61 = tvar::TrpcVarGroup::TryGet("/w61", true);
    ASSERT_TRUE(value61->isObject());
    ASSERT_TRUE(value61->isMember("now"));
    w61_ptr = w61.GetSeries();
    ASSERT_TRUE(w61_ptr.lock());
    Window<IntRecorder> w62(parent, "w62", &c6);
    auto value62 = tvar::TrpcVarGroup::TryGet("/a/b/w62", true);
    ASSERT_TRUE(value62->isObject());
    ASSERT_TRUE(value62->isMember("now"));
    Window<PassiveStatus<uint64_t>> w71("w71", &c7);
    auto value71 = tvar::TrpcVarGroup::TryGet("/w71", true);
    ASSERT_TRUE(value71->isObject());
    ASSERT_TRUE(value71->isMember("now"));
    w71_ptr = w71.GetSeries();
    ASSERT_TRUE(w71_ptr.lock());
    Window<PassiveStatus<uint64_t>> w72(parent, "w72", &c7);
    auto value72 = tvar::TrpcVarGroup::TryGet("/a/b/w72", true);
    ASSERT_TRUE(value72->isObject());
    ASSERT_TRUE(value72->isMember("now"));
  }

  std::this_thread::sleep_for(std::chrono::seconds(3));
  ASSERT_FALSE(w11_ptr.lock());
  ASSERT_FALSE(w21_ptr.lock());
  ASSERT_FALSE(w31_ptr.lock());
  ASSERT_FALSE(w41_ptr.lock());
  ASSERT_FALSE(w51_ptr.lock());
  ASSERT_FALSE(w61_ptr.lock());
  ASSERT_FALSE(w71_ptr.lock());
}

/// @brief Test for PerSecond exposed.
TEST_F(TestWindow, PerSecondExposed) {
  Counter<int64_t> c1;
  Gauge<int64_t> c2;
  Maxer<int64_t> c3;
  Miner<int64_t> c4;
  Averager<int64_t> c5;
  IntRecorder c6;
  PassiveStatus<uint64_t> c7([begin = GetSystemMicroseconds()]() {
    if (auto end = GetSystemMicroseconds(); end > begin) {
      return end - begin;
    }
    return static_cast<uint64_t>(0);
  });

  std::weak_ptr<PerSecond<Counter<int64_t>>::SeriesSamplerType> w11_ptr;
  std::weak_ptr<PerSecond<Gauge<int64_t>>::SeriesSamplerType> w21_ptr;
  std::weak_ptr<PerSecond<Maxer<int64_t>>::SeriesSamplerType> w31_ptr;
  std::weak_ptr<PerSecond<Miner<int64_t>>::SeriesSamplerType> w41_ptr;
  std::weak_ptr<PerSecond<PassiveStatus<uint64_t>>::SeriesSamplerType> w71_ptr;

  auto parent = trpc::tvar::TrpcVarGroup::FindOrCreate("/a/b");
  {
    PerSecond<Counter<int64_t>> w11("s11", &c1);
    ASSERT_EQ(w11.GetAbsPath(), "/s11");
    auto value11 = tvar::TrpcVarGroup::TryGet("/s11", true);
    ASSERT_TRUE(value11->isObject());
    ASSERT_TRUE(value11->isMember("now"));
    w11_ptr = w11.GetSeries();
    ASSERT_TRUE(w11_ptr.lock());
    PerSecond<Counter<int64_t>> w12(parent, "s12", &c1);
    ASSERT_EQ(w12.GetAbsPath(), "/a/b/s12");
    auto value12 = tvar::TrpcVarGroup::TryGet("/a/b/s12", true);
    ASSERT_TRUE(value12->isObject());
    ASSERT_TRUE(value12->isMember("now"));
    PerSecond<Gauge<int64_t>> w21("s21", &c2);
    auto value21 = tvar::TrpcVarGroup::TryGet("/s21", true);
    ASSERT_TRUE(value21->isObject());
    ASSERT_TRUE(value21->isMember("now"));
    w21_ptr = w21.GetSeries();
    ASSERT_TRUE(w21_ptr.lock());
    PerSecond<Gauge<int64_t>> w22(parent, "s22", &c2);
    auto value22 = tvar::TrpcVarGroup::TryGet("/a/b/s22", true);
    ASSERT_TRUE(value22->isObject());
    ASSERT_TRUE(value22->isMember("now"));
    PerSecond<Maxer<int64_t>> w31("s31", &c3);
    auto value31 = tvar::TrpcVarGroup::TryGet("/s31", true);
    ASSERT_TRUE(value31->isObject());
    ASSERT_TRUE(value31->isMember("now"));
    w31_ptr = w31.GetSeries();
    ASSERT_TRUE(w31_ptr.lock());
    PerSecond<Maxer<int64_t>> w32(parent, "s32", &c3);
    auto value32 = tvar::TrpcVarGroup::TryGet("/a/b/s32", true);
    ASSERT_TRUE(value32->isObject());
    ASSERT_TRUE(value32->isMember("now"));
    PerSecond<Miner<int64_t>> w41("s41", &c4);
    auto value41 = tvar::TrpcVarGroup::TryGet("/s41", true);
    ASSERT_TRUE(value41->isObject());
    ASSERT_TRUE(value41->isMember("now"));
    w41_ptr = w41.GetSeries();
    ASSERT_TRUE(w41_ptr.lock());
    PerSecond<Miner<int64_t>> w42(parent, "s42", &c4);
    auto value42 = tvar::TrpcVarGroup::TryGet("/a/b/s42", true);
    ASSERT_TRUE(value42->isObject());
    ASSERT_TRUE(value42->isMember("now"));
    PerSecond<PassiveStatus<uint64_t>> w71("s71", &c7);
    auto value71 = tvar::TrpcVarGroup::TryGet("/s71", true);
    ASSERT_TRUE(value71->isObject());
    ASSERT_TRUE(value71->isMember("now"));
    w71_ptr = w71.GetSeries();
    ASSERT_TRUE(w71_ptr.lock());
    PerSecond<PassiveStatus<uint64_t>> w72(parent, "s72", &c7);
    auto value72 = tvar::TrpcVarGroup::TryGet("/a/b/s72", true);
    ASSERT_TRUE(value72->isObject());
    ASSERT_TRUE(value72->isMember("now"));
  }

  std::this_thread::sleep_for(std::chrono::seconds(3));
  ASSERT_FALSE(w11_ptr.lock());
  ASSERT_FALSE(w21_ptr.lock());
  ASSERT_FALSE(w31_ptr.lock());
  ASSERT_FALSE(w41_ptr.lock());
  ASSERT_FALSE(w71_ptr.lock());
}

/// @brief Test for Window not abort on same path.
TEST_F(TestWindow, NotAbortOnSamePath) {
  const std::string kRelPath("w11");
  const std::string kAbsPath("/w11");
  trpc::tvar::SetWhetherToAbortOnSamePath(false);
  Counter<int64_t> counter;
  auto window = std::make_unique<Window<Counter<int64_t>>>(kRelPath, &counter);
  ASSERT_TRUE(window->IsExposed());
  ASSERT_EQ(window->GetAbsPath(), kAbsPath);
  {
    auto value = tvar::TrpcVarGroup::TryGet(kAbsPath, true);
    ASSERT_TRUE(value->isObject());
    ASSERT_TRUE(value->isMember("now"));
  }
  auto repetitive_window = std::make_unique<Window<Counter<int64_t>>>(kRelPath, &counter);
  ASSERT_FALSE(repetitive_window->IsExposed());
  ASSERT_TRUE(repetitive_window->GetAbsPath().empty());
  window.reset();
  window = std::make_unique<Window<Counter<int64_t>>>(kRelPath, &counter);
  ASSERT_TRUE(window->IsExposed());
  ASSERT_EQ(window->GetAbsPath(), kAbsPath);
  {
    auto value = tvar::TrpcVarGroup::TryGet(kAbsPath, true);
    ASSERT_TRUE(value->isObject());
    ASSERT_TRUE(value->isMember("now"));
  }
}

/// @brief Test for PerSecond not abort on same path.
TEST_F(TestWindow, PerSecondNotAbortOnSamePath) {
  const std::string kRelPath("s11");
  const std::string kAbsPath("/s11");
  trpc::tvar::SetWhetherToAbortOnSamePath(false);
  Counter<int64_t> counter;
  auto per_second = std::make_unique<PerSecond<Counter<int64_t>>>(kRelPath, &counter);
  ASSERT_TRUE(per_second->IsExposed());
  ASSERT_EQ(per_second->GetAbsPath(), kAbsPath);
  {
    auto value = tvar::TrpcVarGroup::TryGet(kAbsPath, true);
    ASSERT_TRUE(value->isObject());
    ASSERT_TRUE(value->isMember("now"));
  }
  auto repetitive_per_second = std::make_unique<PerSecond<Counter<int64_t>>>(kRelPath, &counter);
  ASSERT_FALSE(repetitive_per_second->IsExposed());
  ASSERT_TRUE(repetitive_per_second->GetAbsPath().empty());
  per_second.reset();
  per_second = std::make_unique<PerSecond<Counter<int64_t>>>(kRelPath, &counter);
  ASSERT_TRUE(per_second->IsExposed());
  ASSERT_EQ(per_second->GetAbsPath(), kAbsPath);
  {
    auto value = tvar::TrpcVarGroup::TryGet(kAbsPath, true);
    ASSERT_TRUE(value->isObject());
    ASSERT_TRUE(value->isMember("now"));
  }
}

}  // namespace trpc::testing
