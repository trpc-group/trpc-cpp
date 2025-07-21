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

#include "trpc/runtime/iomodel/reactor/default/reactor_impl.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/util/thread/latch.h"
#include "trpc/util/time.h"

namespace trpc::testing {

class ReactorImplTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ReactorImpl::Options reactor_options;
    reactor_options.id = 0;

    reactor_ = std::make_unique<ReactorImpl>(reactor_options);
    reactor_->Initialize();

    Latch l(1);
    std::thread t([this, &l]() {
      l.count_down();
      this->reactor_->Run();
    });

    thread_ = std::move(t);

    l.wait();
  }

  void TearDown() override {
    reactor_->Stop();
    thread_.join();
    reactor_->Destroy();
  }

 protected:
  std::unique_ptr<Reactor> reactor_;
  std::thread thread_;
};

TEST_F(ReactorImplTest, TestReactorImpl) {
  ASSERT_EQ(reactor_->Id(), 0);
  ASSERT_EQ(reactor_->Name(), "default_impl");

  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;

  Reactor::Task reactor_task = [this, &ret, &mutex, &cond] {
    Reactor::TimerExecutor executor1 = [&ret, &mutex, &cond] {
      std::unique_lock<std::mutex> lock(mutex);
      ret += 1;
      cond.notify_all();
    };

    uint64_t expiration = trpc::time::GetMilliSeconds() + 300;
    uint64_t timer_id1 = this->reactor_->AddTimerAt(expiration, 0, std::move(executor1));
    this->reactor_->DetachTimer(timer_id1);

    Reactor::TimerExecutor executor2 = [&ret, &mutex, &cond] {
      std::unique_lock<std::mutex> lock(mutex);
      ret += 1;
      cond.notify_all();
    };

    uint64_t timer_id2 = this->reactor_->AddTimerAfter(500, 0, std::move(executor2));
    this->reactor_->DetachTimer(timer_id2);
  };

  reactor_->SubmitTask(std::move(reactor_task));

  std::cout << "ret:" << ret << std::endl;
  {
    std::unique_lock <std::mutex> lock(mutex);
    while (ret != 2)
      cond.wait(lock);
  }

  ASSERT_EQ(ret, 2);
}

TEST_F(ReactorImplTest, Priority) {
  ASSERT_EQ(reactor_->Id(), 0);
  ASSERT_EQ(reactor_->Name(), "default_impl");

  int counter = 0;

  EXPECT_TRUE(reactor_->SubmitTask([&counter]() { ++counter; }, Reactor::Priority::kLowest));
  EXPECT_TRUE(reactor_->SubmitTask([&counter]() { ++counter; }, Reactor::Priority::kLow));
  EXPECT_TRUE(reactor_->SubmitTask([&counter]() { ++counter; }, Reactor::Priority::kNormal));
  EXPECT_TRUE(reactor_->SubmitTask([&counter]() { ++counter; }, Reactor::Priority::kHigh));

  while (counter != 4) {
    ::usleep(100000);
  }

  ASSERT_EQ(counter, 4);
}

}  // namespace trpc::testing
