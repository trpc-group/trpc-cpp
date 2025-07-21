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

#include "trpc/runtime/iomodel/reactor/default/timer_queue.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/runtime/iomodel/reactor/default/reactor_impl.h"
#include "trpc/util/thread/latch.h"
#include "trpc/util/time.h"

namespace trpc::testing {

using namespace std::literals;

class TimerQueueTest : public ::testing::Test {
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

TEST_F(TimerQueueTest, All) {
  int ret = 0;
  int repeat_ret = 0;
  std::mutex mutex;
  std::condition_variable cond;

  Reactor::Task reactor_task = [this, &ret, &repeat_ret, &mutex, &cond] {
    Reactor::TimerExecutor executor1 = [&ret, &mutex, &cond] {
      std::unique_lock<std::mutex> lock(mutex);
      ret += 1;
      std::cout << "ret:" << ret << std::endl;
      cond.notify_all();
    };

    uint64_t expiration = trpc::time::GetMilliSeconds() + 300;
    uint64_t timer_id1 = this->reactor_->AddTimerAt(expiration, 0, std::move(executor1));
    this->reactor_->DetachTimer(timer_id1);

    Reactor::TimerExecutor executor2 = [&ret, &mutex, &cond] {
      std::unique_lock<std::mutex> lock(mutex);
      ret += 1;
      std::cout << "cancel ret:" << ret << std::endl;
      cond.notify_all();
    };

    uint64_t timer_id2 = this->reactor_->AddTimerAfter(500, 0, std::move(executor2));
    this->reactor_->CancelTimer(timer_id2);

    Reactor::TimerExecutor executor3 = [&repeat_ret, &mutex, &cond] {
      std::unique_lock<std::mutex> lock(mutex);
      repeat_ret += 1;
      std::cout << "repeat_ret:" << repeat_ret << std::endl;
      cond.notify_all();
    };

    uint64_t timer_id3 = this->reactor_->AddTimerAfter(0, 500, std::move(executor3));
    this->reactor_->DetachTimer(timer_id3);
  };

  reactor_->SubmitTask(std::move(reactor_task));

  {
    std::unique_lock<std::mutex> lock(mutex);
    while (ret < 1 && repeat_ret < 3) cond.wait_for(lock, 100ms);
  }

  ASSERT_EQ(ret, 1);
  ASSERT_GE(repeat_ret, 1);
}

}  // namespace trpc::testing
