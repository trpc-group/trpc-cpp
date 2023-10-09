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

#include <memory>
#include <string>

#include "trpc/future/async_lock/with_lock.h"

#include "trpc/common/async_timer.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/future/async_lock/rwlock.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/runtime.h"

#include "gtest/gtest.h"

namespace trpc {

class WithLockTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    ASSERT_EQ(TrpcConfig::GetInstance()->Init("trpc/runtime/threadmodel/testing/merge.yaml"), 0);
    trpc::merge::StartRuntime();
  }

  static void TearDownTestCase() {
    trpc::merge::TerminateRuntime();
  }

 protected:
  size_t count_ = 0;
  bool flag_  = false;
  std::string merge_thread_model_ = "default_instance";
};

TEST(WithLock, Read) {
  RWLock l;

  size_t count = 0;
  WithLock(l.ForRead(), [&count]() {
    EXPECT_EQ(count, 0);
  });
}

TEST(WithLock, Write) {
  RWLock l;
  size_t count = 0;
  WithLock(l.ForWrite(), [&count]() {
    count += 1;
  });
  EXPECT_EQ(count, 1);

  bool flag = false;
  WithLock(l.ForRead(), [&flag] {
    flag = true;
  });

  EXPECT_TRUE(flag);
}

TEST_F(WithLockTest, Read_with_reactor) {
  auto timer = std::make_shared<AsyncTimer>();

  RWLock* lock = nullptr;

  Reactor* reactor = runtime::GetReactor(0, merge_thread_model_);

  reactor->SubmitTask([this, &timer, &lock] {
    lock = new RWLock();
    // A aquires read lock (instantly).
    WithLock(lock->ForRead(), [this, &timer]() {
      // A succeeded to aquire lock, and tag it's locking.
      this->flag_ = true;
      EXPECT_EQ(this->count_, 0);
      // before unlock, something takes a lot of time.
      return timer->After(10).Then([this] {
        // A's work is about to be done, set it to non-locking.
        this->flag_ = false;
        return MakeReadyFuture<>();
      });
    // WithLock should unlock automatically.
    });

    // B aquires read lock at the same time (approximately).
    WithLock(lock->ForRead(), [this]() {
      EXPECT_EQ(this->count_, 0);
      // Here means B aquired lock successfully
      // and flag showed someone(A) aquired this lock, and did not unlock yet.
      EXPECT_TRUE(this->flag_);
    });
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // when A is done, flag has been reset.
  EXPECT_FALSE(flag_);
  delete lock;
}

TEST_F(WithLockTest, Write_with_reactor) {
  auto timer = std::make_shared<AsyncTimer>();

  RWLock* lock = nullptr;

  Reactor* reactor = runtime::GetReactor(0, merge_thread_model_);

  reactor->SubmitTask([this, &timer, &lock] {
    lock = new RWLock();
    // A aquires write lock (instantly).
    WithLock(lock->ForWrite(), [this, &timer]() {
      // locks for 10ms, modify count.
      return timer->After(10).Then([this] {
        this->count_ += 1;
        return MakeReadyFuture<>();
      });
    // WithLock should unlock automatically.
    });

    // A has not modified count yet.
    EXPECT_EQ(this->count_, 0);

    // B aquires read lock and it should be waiting.
    WithLock(lock->ForRead(), [this]() {
      // Here means B aquired lock successfully.
      EXPECT_EQ(this->count_, 1);
      EXPECT_FALSE(this->flag_);
      this->flag_ = true;
    });
    EXPECT_EQ(lock->Waiters(), 1);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Means B worked.
  EXPECT_TRUE(this->flag_);
  delete lock;
}

TEST_F(WithLockTest, Write_throw_exception_with_reactor) {
  RWLock* lock = nullptr;

  Reactor* reactor = runtime::GetReactor(0, merge_thread_model_);

  reactor->SubmitTask([this, &lock] {
    lock = new RWLock();
    // A aquires write lock (instantly).
    WithLock(lock->ForWrite(), [this]() {
      // Something bad happened.
      throw std::runtime_error("Write lock failed");
      // WithLock should unlock automatically.
    }).Then([] (Future<>&& ft) {
      EXPECT_TRUE(ft.IsFailed());
    });

    // B aquires read lock and it should be waiting.
    auto f2 = WithLock(lock->ForRead(), [this]() {
      // Here means B aquired lock successfully.
      EXPECT_FALSE(this->flag_);
      this->flag_ = true;
      return MakeReadyFuture<int>(6);
    });

    EXPECT_FALSE(f2.IsReady());
    EXPECT_EQ(lock->Waiters(), 1);
    f2.Then([] (Future<int>&& ft) {
      EXPECT_FALSE(ft.IsFailed());
      EXPECT_EQ(ft.GetConstValue(), 6);
    });
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Means B worked.
  EXPECT_TRUE(flag_);
  delete lock;
}

TEST_F(WithLockTest, Thread_safe_with_reactor) {
  RWLock* lock = nullptr;

  Reactor* reactor = runtime::GetReactor(0, merge_thread_model_);

  // Lock & Unlock on the same thread that the lock object was created on.
  reactor->SubmitTask([this, &lock] {
    lock = new RWLock();
    auto f1 = WithLock(lock->ForWrite(), [this] {
      EXPECT_FALSE(this->flag_);
      this->flag_ = true;
    });
    EXPECT_FALSE(f1.IsFailed());
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_TRUE(this->flag_);

  // On other thread, can not lock.
  auto f2 = WithLock(lock->ForRead(), [] {});

  EXPECT_TRUE(f2.IsFailed());
  EXPECT_STREQ(f2.GetException().what(), "Not on the same worker thread");

  // On other thread, can not lock.
  auto f3 = WithLock(lock->ForWrite(), [] {});

  EXPECT_TRUE(f3.IsFailed());
  EXPECT_STREQ(f3.GetException().what(), "Not on the same worker thread");

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  delete lock;

  // Lock on the same thread, but unlock on others.
  reactor->SubmitTask([this, &lock] {
    lock = new RWLock();
    auto f4 = lock->WriteLock();
    EXPECT_FALSE(f4.IsFailed());

    lock->ReadLock();
    EXPECT_EQ(lock->Waiters(), 1);

    // Does not unlock.
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // This should fail (on non-worker thread).
  EXPECT_FALSE(lock->WriteUnLock());

  Reactor* reactor_other = runtime::GetReactor(1, merge_thread_model_);

  // This should also fail (on other worker thread)
  reactor_other->SubmitTask([&lock] {
    // Do not check lock attribute on other thread, only for test.
    EXPECT_EQ(lock->Waiters(), 1);
    EXPECT_FALSE(lock->WriteUnLock());
    EXPECT_EQ(lock->Waiters(), 1);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  reactor->SubmitTask([&lock] {
    EXPECT_EQ(lock->Waiters(), 1);
    EXPECT_TRUE(lock->WriteUnLock());
    EXPECT_EQ(lock->Waiters(), 0);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(WithLockTest, WithLock_return_type_with_reactor) {
  Reactor* reactor = runtime::GetReactor(0, merge_thread_model_);

  // Lock & Unlock on the same thread that the lock object was created on.
  reactor->SubmitTask([this] {
    auto lock = std::make_shared<RWLock>();

    WithLock(lock->ForRead(), [this, lock] () {
      // Return Future<T>
      return MakeReadyFuture<int>(6);
    }).Then([this] (Future<int>&& fut) {
      EXPECT_EQ(std::get<0>(fut.GetValue()), 6);
    });

    WithLock(lock->ForRead(), [this, lock] () {
      // Return T.
      return 16;
    }).Then([this] (Future<int>&& fut) {
      EXPECT_EQ(std::get<0>(fut.GetValue()), 16);
    });

    WithLock(lock->ForWrite(), [this, lock] () {
      // return void
      return;
    }).Then([this] (Future<>&& fut) {
      EXPECT_FALSE(this->flag_);
      this->flag_ = true;
      return MakeReadyFuture<>();
    }).Then([this] () {
      EXPECT_TRUE(this->flag_);
    });
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

}  // namespace trpc
