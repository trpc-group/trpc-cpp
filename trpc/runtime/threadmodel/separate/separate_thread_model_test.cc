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

#include "trpc/runtime/threadmodel/separate/separate_thread_model.h"

#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/runtime/iomodel/reactor/common/epoll_poller.h"
#include "trpc/runtime/iomodel/reactor/default/reactor_impl.h"
#include "trpc/runtime/threadmodel/common/msg_task.h"
#include "trpc/runtime/threadmodel/separate/non_steal/non_steal_scheduling.h"
#include "trpc/runtime/threadmodel/separate/steal/steal_scheduling.h"
#include "trpc/util/object_pool/object_pool_ptr.h"
#include "trpc/util/time.h"

namespace trpc {

namespace testing {

enum ScheduleType {
  kNonCoroutine,
  kTaskflow,
};

class TestSeparateThreadModel : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    RegisterSeparateScheduling();
    separate_thread_model_ = CreateSeparateThreadModel(schedule_type_);
    separate_thread_model_->Start();
  }

  static void RegisterSeparateScheduling() {
    SeparateSchedulingCreateFunction sche_create_func = []() {
      // Register the non-coroutine scheduler creation function
      separate::NonStealScheduling::Options option;
      option.group_name = "group1";
      option.worker_thread_num = 2;
      option.global_queue_size = 32;
      option.local_queue_size = 4;
      std::unique_ptr<SeparateScheduling> sep_sche =
          std::make_unique<separate::NonStealScheduling>(std::move(option));
      return sep_sche;
    };
    RegisterSeparateSchedulingImpl(std::string(kNonStealScheduling), std::move(sche_create_func));

    // Register the taskflow scheduler creation function
    sche_create_func = []() {
      separate::StealScheduling::Options option;
      option.group_name = "group1";
      option.worker_thread_num = 2;
      option.global_queue_size = 32;
      std::unique_ptr<SeparateScheduling> sep_sche = std::make_unique<separate::StealScheduling>(std::move(option));
      return sep_sche;
    };
    RegisterSeparateSchedulingImpl(std::string(kStealScheduling), std::move(sche_create_func));
  }

  static SeparateThreadModel* CreateSeparateThreadModel(ScheduleType type) {
    SeparateThreadModel::Options thread_model_options;
    thread_model_options.group_id = 0;
    thread_model_options.group_name = "group1";
    std::string_view scheduling_name = (type == kTaskflow ? kStealScheduling : kNonStealScheduling);
    thread_model_options.scheduling_name = scheduling_name;
    thread_model_options.io_thread_num = 2;
    thread_model_options.handle_thread_num = 2;

    return new SeparateThreadModel(std::move(thread_model_options));
  }

  static void TearDownTestCase() {
    separate_thread_model_->Terminate();
    delete separate_thread_model_;
    separate_thread_model_ = nullptr;
  }

  static void SetScheduleType(ScheduleType type) { schedule_type_ = type; }

 protected:
  static SeparateThreadModel* separate_thread_model_;
  static ScheduleType schedule_type_;
};

SeparateThreadModel* TestSeparateThreadModel::separate_thread_model_ = nullptr;
ScheduleType TestSeparateThreadModel::schedule_type_ = kNonCoroutine;

// Test submit an io task on a non-framework thread and randomly assign it to run on a framework io thread
TEST_F(TestSeparateThreadModel, SubmitIoTask1OnSeparate) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id tid;

  MsgTask* task = object_pool::New<MsgTask>();
  task->task_type = trpc::runtime::kResponseMsg;
  task->param = nullptr;
  task->handler = [&ret, &mutex, &cond, &tid]() {
    tid = std::this_thread::get_id();
    std::unique_lock<std::mutex> lock(mutex);
    ret = 1;
    cond.notify_all();
  };
  task->group_id = separate_thread_model_->GroupId();

  separate_thread_model_->SubmitIoTask(std::move(task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  EXPECT_EQ(ret, 1);
  EXPECT_EQ(std::this_thread::get_id() != tid, true);
}

// Test submit an io task on a non-framework thread and run it on the specified io thread
TEST_F(TestSeparateThreadModel, SubmitIoTask2OnSeparate) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id tid;
  int thread_index = 0;

  MsgTask* task = trpc::object_pool::New<MsgTask>();
  task->task_type = trpc::runtime::kResponseMsg;
  task->param = nullptr;
  task->handler = [&ret, &mutex, &cond, &tid, &thread_index]() {
    tid = std::this_thread::get_id();
    std::unique_lock<std::mutex> lock(mutex);
    ret = 1;
    thread_index = WorkerThread::GetCurrentWorkerThread()->Id();
    cond.notify_all();
  };
  task->group_id = 0;
  task->dst_thread_key = 1;

  separate_thread_model_->SubmitIoTask(std::move(task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  EXPECT_EQ(ret, 1);
  EXPECT_EQ(std::this_thread::get_id() != tid, true);
  EXPECT_EQ(thread_index, 1);
}

// Submit an io task on a framework io thread and run this task on the same io thread
TEST_F(TestSeparateThreadModel, SubmitIoTask3OnSeparate) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id tid;
  int thread_index = 0;
  int task_execute_thread_index = -1;

  Reactor::Task reactor_task = [&ret, &mutex, &cond, &tid, &thread_index, &task_execute_thread_index] {
    task_execute_thread_index = WorkerThread::GetCurrentWorkerThread()->Id();

    MsgTask* task = trpc::object_pool::New<MsgTask>();
    task->task_type = trpc::runtime::kResponseMsg;
    task->param = nullptr;
    task->handler = [&ret, &mutex, &cond, &tid, &thread_index]() {
      tid = std::this_thread::get_id();
      std::unique_lock<std::mutex> lock(mutex);
      ret = 1;
      thread_index = WorkerThread::GetCurrentWorkerThread()->Id();
      cond.notify_all();
    };
    task->group_id = 0;

    TestSeparateThreadModel::separate_thread_model_->SubmitIoTask(std::move(task));
  };

  TestSeparateThreadModel::separate_thread_model_->GetReactor(thread_index)->SubmitTask(std::move(reactor_task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  EXPECT_EQ(ret, 1);
  EXPECT_EQ(std::this_thread::get_id() != tid, true);
  EXPECT_EQ(thread_index, 0);
  EXPECT_EQ(task_execute_thread_index, thread_index);
}

// Test submit an io task on a framework io thread and specify to run it on the thread that submitted this io task
TEST_F(TestSeparateThreadModel, SubmitIoTask4OnSeparate) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id tid;
  int thread_index = 0;
  int task_execute_thread_index = -1;

  Reactor::Task reactor_task = [&ret, &mutex, &cond, &tid, &thread_index, &task_execute_thread_index] {
    task_execute_thread_index = WorkerThread::GetCurrentWorkerThread()->Id();

    MsgTask* task = trpc::object_pool::New<MsgTask>();
    task->task_type = trpc::runtime::kResponseMsg;
    task->param = nullptr;
    task->handler = [&ret, &mutex, &cond, &tid, &thread_index]() {
      tid = std::this_thread::get_id();
      std::unique_lock<std::mutex> lock(mutex);
      ret = 1;
      thread_index = WorkerThread::GetCurrentWorkerThread()->Id();
      cond.notify_all();
    };
    task->group_id = 0;
    task->dst_thread_key = 0;

    TestSeparateThreadModel::separate_thread_model_->SubmitIoTask(std::move(task));
  };

  TestSeparateThreadModel::separate_thread_model_->GetReactor(thread_index)->SubmitTask(std::move(reactor_task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  EXPECT_EQ(ret, 1);
  EXPECT_EQ(std::this_thread::get_id() != tid, true);
  EXPECT_EQ(thread_index, 0);
  EXPECT_EQ(task_execute_thread_index, thread_index);
}

// Test submit an io task on a framework io thread and specify to run it on another io thread
TEST_F(TestSeparateThreadModel, SubmitIoTask5OnSeparate) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id tid;
  int thread_index = 0;
  int task_execute_thread_index = -1;

  Reactor::Task reactor_task = [&ret, &mutex, &cond, &tid, &thread_index, &task_execute_thread_index] {
    task_execute_thread_index = WorkerThread::GetCurrentWorkerThread()->Id();

    MsgTask* task = trpc::object_pool::New<MsgTask>();
    task->task_type = trpc::runtime::kResponseMsg;
    task->param = nullptr;
    task->handler = [&ret, &mutex, &cond, &tid, &thread_index]() {
      tid = std::this_thread::get_id();
      std::unique_lock<std::mutex> lock(mutex);
      ret = 1;
      thread_index = WorkerThread::GetCurrentWorkerThread()->Id();
      cond.notify_all();
    };
    task->group_id = 0;
    task->dst_thread_key = 1;

    TestSeparateThreadModel::separate_thread_model_->SubmitIoTask(std::move(task));
  };

  TestSeparateThreadModel::separate_thread_model_->GetReactor(thread_index)->SubmitTask(std::move(reactor_task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  EXPECT_EQ(ret, 1);
  EXPECT_EQ(std::this_thread::get_id() != tid, true);
  EXPECT_EQ(thread_index, 1);
  EXPECT_EQ(task_execute_thread_index, 0);
  EXPECT_EQ(task_execute_thread_index != thread_index, true);
}

// Test submit an io task on a framework handle thread and randomly run it on a framework io thread
TEST_F(TestSeparateThreadModel, SubmitIoTask6OnSeparate) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id tid;

  MsgTask* req_task = trpc::object_pool::New<MsgTask>();
  req_task->task_type = trpc::runtime::kRequestMsg;
  req_task->param = nullptr;
  req_task->handler = [&ret, &mutex, &cond, &tid]() {
    MsgTask* rsp_task = trpc::object_pool::New<MsgTask>();
    rsp_task->task_type = trpc::runtime::kResponseMsg;
    rsp_task->param = nullptr;
    rsp_task->handler = [&ret, &mutex, &cond, &tid]() {
      tid = std::this_thread::get_id();
      std::unique_lock<std::mutex> lock(mutex);
      ret = 1;
      cond.notify_all();
    };
    rsp_task->group_id = 0;

    TestSeparateThreadModel::separate_thread_model_->SubmitIoTask(std::move(rsp_task));
  };

  TestSeparateThreadModel::separate_thread_model_->SubmitHandleTask(std::move(req_task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  EXPECT_EQ(ret, 1);
  EXPECT_EQ(std::this_thread::get_id() != tid, true);
}

// Test submit an io task on a framework handle thread and run it on a specified framework io thread
TEST_F(TestSeparateThreadModel, SubmitIoTask7OnSeparate) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id tid;
  int thread_index = 0;

  MsgTask* req_task = trpc::object_pool::New<MsgTask>();
  req_task->task_type = trpc::runtime::kRequestMsg;
  req_task->param = nullptr;
  req_task->handler = [&ret, &mutex, &cond, &tid, &thread_index]() {
    MsgTask* rsp_task = trpc::object_pool::New<MsgTask>();
    rsp_task->task_type = trpc::runtime::kResponseMsg;
    rsp_task->param = nullptr;
    rsp_task->handler = [&ret, &mutex, &cond, &tid, &thread_index]() {
      tid = std::this_thread::get_id();
      std::unique_lock<std::mutex> lock(mutex);
      ret = 1;
      thread_index = WorkerThread::GetCurrentWorkerThread()->Id();
      cond.notify_all();
    };
    rsp_task->group_id = 0;
    rsp_task->dst_thread_key = 1;

    TestSeparateThreadModel::separate_thread_model_->SubmitIoTask(std::move(rsp_task));
  };

  TestSeparateThreadModel::separate_thread_model_->SubmitHandleTask(std::move(req_task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  EXPECT_EQ(ret, 1);
  EXPECT_EQ(std::this_thread::get_id() != tid, true);
  EXPECT_EQ(thread_index, 1);
}

// Test submit a handle task on a non-framework thread and randomly run it on a framework handle thread
TEST_F(TestSeparateThreadModel, SubmitHandleTask1OnSeparate) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id tid;

  MsgTask* task = trpc::object_pool::New<MsgTask>();
  task->task_type = trpc::runtime::kRequestMsg;
  task->param = nullptr;
  task->handler = [&ret, &mutex, &cond, &tid]() {
    tid = std::this_thread::get_id();
    std::unique_lock<std::mutex> lock(mutex);
    ret = 1;
    cond.notify_all();
  };
  task->group_id = 0;

  separate_thread_model_->SubmitHandleTask(std::move(task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  EXPECT_EQ(ret, 1);
  EXPECT_EQ(std::this_thread::get_id() != tid, true);
}

// Test submit a handle task on a non-framework thread and run it on a specified framework handle thread
TEST_F(TestSeparateThreadModel, SubmitHandleTask2OnSeparate) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id tid;
  int thread_index = 0;

  MsgTask* task = trpc::object_pool::New<MsgTask>();
  task->task_type = trpc::runtime::kRequestMsg;
  task->param = nullptr;
  task->handler = [&ret, &mutex, &cond, &tid, &thread_index]() {
    tid = std::this_thread::get_id();
    std::unique_lock<std::mutex> lock(mutex);
    ret = 1;
    thread_index = WorkerThread::GetCurrentWorkerThread()->Id();
    cond.notify_all();
  };
  task->group_id = 0;
  task->dst_thread_key = 1;

  separate_thread_model_->SubmitHandleTask(std::move(task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  EXPECT_EQ(ret, 1);
  EXPECT_EQ(std::this_thread::get_id() != tid, true);
  if (schedule_type_ == kNonCoroutine) {
    EXPECT_EQ(thread_index, 1);
  }
}

// Test submit a handle task on a framework io thread and randomly run it on a framework handle thread
TEST_F(TestSeparateThreadModel, SubmitHandleTask3OnSeparate) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id handle_tid;
  std::thread::id io_tid;

  Reactor::Task reactor_task = [&ret, &mutex, &cond, &io_tid, &handle_tid] {
    io_tid = std::this_thread::get_id();

    MsgTask* task = trpc::object_pool::New<MsgTask>();
    task->task_type = trpc::runtime::kRequestMsg;
    task->param = nullptr;
    task->handler = [&ret, &mutex, &cond, &io_tid, &handle_tid]() {
      handle_tid = std::this_thread::get_id();
      std::unique_lock<std::mutex> lock(mutex);
      ret = 1;
      cond.notify_all();
    };
    task->group_id = 0;

    TestSeparateThreadModel::separate_thread_model_->SubmitHandleTask(std::move(task));
  };

  TestSeparateThreadModel::separate_thread_model_->GetReactor()->SubmitTask(std::move(reactor_task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  EXPECT_EQ(ret, 1);
  EXPECT_EQ(std::this_thread::get_id() != io_tid, true);
  EXPECT_EQ(io_tid != handle_tid, true);
}

// Test submit a handle task on a framework io thread and run it on a specified framework handle thread
TEST_F(TestSeparateThreadModel, SubmitHandleTask4OnSeparate) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id handle_tid;
  std::thread::id io_tid;
  int thread_index = 0;

  Reactor::Task reactor_task = [&ret, &mutex, &cond, &io_tid, &handle_tid, &thread_index] {
    io_tid = std::this_thread::get_id();

    MsgTask* task = trpc::object_pool::New<MsgTask>();
    task->task_type = trpc::runtime::kRequestMsg;
    task->param = nullptr;
    task->handler = [&ret, &mutex, &cond, &io_tid, &handle_tid, &thread_index]() {
      handle_tid = std::this_thread::get_id();
      std::unique_lock<std::mutex> lock(mutex);
      ret = 1;
      thread_index = WorkerThread::GetCurrentWorkerThread()->Id();
      cond.notify_all();
    };
    task->group_id = 0;
    task->dst_thread_key = 1;

    TestSeparateThreadModel::separate_thread_model_->SubmitHandleTask(std::move(task));
  };

  TestSeparateThreadModel::separate_thread_model_->GetReactor()->SubmitTask(std::move(reactor_task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  EXPECT_EQ(ret, 1);
  EXPECT_EQ(std::this_thread::get_id() != io_tid, true);
  EXPECT_EQ(io_tid != handle_tid, true);
  if (schedule_type_ != kTaskflow) {
    EXPECT_EQ(thread_index, 1);
  }
}

// Test submit a handle task on a framework handle thread and run it on a specified framework handle thread
TEST_F(TestSeparateThreadModel, SubmitHandleTask5OnSeparate) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id tid;
  std::thread::id handle_tid;

  MsgTask* req_task = trpc::object_pool::New<MsgTask>();
  req_task->task_type = trpc::runtime::kRequestMsg;
  req_task->param = nullptr;
  req_task->handler = [&ret, &mutex, &cond, &tid, &handle_tid]() {
    tid = std::this_thread::get_id();

    MsgTask* rsp_task = trpc::object_pool::New<MsgTask>();
    rsp_task->task_type = trpc::runtime::kRequestMsg;
    rsp_task->param = nullptr;
    rsp_task->handler = [&ret, &mutex, &cond, &handle_tid]() {
      handle_tid = std::this_thread::get_id();
      std::unique_lock<std::mutex> lock(mutex);
      ret = 1;
      cond.notify_all();
    };
    rsp_task->group_id = 0;
    rsp_task->dst_thread_key = 1;

    TestSeparateThreadModel::separate_thread_model_->SubmitHandleTask(std::move(rsp_task));
  };
  req_task->dst_thread_key = 0;
  TestSeparateThreadModel::separate_thread_model_->SubmitHandleTask(std::move(req_task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  EXPECT_EQ(ret, 1);
  EXPECT_EQ(std::this_thread::get_id() != tid, true);
  if (schedule_type_ != kTaskflow) {
    EXPECT_EQ(handle_tid != tid, true);
  }
}

// Test submit a handle task on a framework handle thread and specify to run it on the current framework handle thread
TEST_F(TestSeparateThreadModel, SubmitHandleTask6OnSeparate) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id tid;
  std::thread::id handle_tid;

  MsgTask* req_task = trpc::object_pool::New<MsgTask>();
  req_task->task_type = trpc::runtime::kRequestMsg;
  req_task->param = nullptr;
  req_task->handler = [&ret, &mutex, &cond, &tid, &handle_tid]() {
    tid = std::this_thread::get_id();

    MsgTask* rsp_task = trpc::object_pool::New<MsgTask>();
    rsp_task->task_type = trpc::runtime::kRequestMsg;
    rsp_task->param = nullptr;
    rsp_task->handler = [&ret, &mutex, &cond, &handle_tid]() {
      handle_tid = std::this_thread::get_id();
      std::unique_lock<std::mutex> lock(mutex);
      ret = 1;
      cond.notify_all();
    };
    rsp_task->group_id = 0;
    rsp_task->dst_thread_key = 0;

    TestSeparateThreadModel::separate_thread_model_->SubmitHandleTask(std::move(rsp_task));
  };
  req_task->dst_thread_key = 0;

  TestSeparateThreadModel::separate_thread_model_->SubmitHandleTask(std::move(req_task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  EXPECT_EQ(ret, 1);
  EXPECT_EQ(std::this_thread::get_id() != tid, true);
  if (schedule_type_ != kTaskflow) {
    EXPECT_EQ(handle_tid == tid, true);
  }
}

// Test submit a handle task on a framework handle thread and run it on a specified framework handle thread
TEST_F(TestSeparateThreadModel, SubmitHandleTask7OnSeparate) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id tid;
  std::thread::id handle_tid;

  MsgTask* req_task = trpc::object_pool::New<MsgTask>();
  req_task->task_type = trpc::runtime::kRequestMsg;
  req_task->param = nullptr;
  req_task->handler = [&ret, &mutex, &cond, &tid, &handle_tid]() {
    tid = std::this_thread::get_id();

    MsgTask* rsp_task = trpc::object_pool::New<MsgTask>();
    rsp_task->task_type = trpc::runtime::kRequestMsg;
    rsp_task->param = nullptr;
    rsp_task->handler = [&ret, &mutex, &cond, &handle_tid]() {
      handle_tid = std::this_thread::get_id();
      std::unique_lock<std::mutex> lock(mutex);
      ret = 1;
      cond.notify_all();
    };
    rsp_task->group_id = 0;
    rsp_task->dst_thread_key = 1;

    TestSeparateThreadModel::separate_thread_model_->SubmitHandleTask(std::move(rsp_task));
  };

  TestSeparateThreadModel::separate_thread_model_->SubmitHandleTask(std::move(req_task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  EXPECT_EQ(ret, 1);
  EXPECT_EQ(std::this_thread::get_id() != tid, true);
  EXPECT_EQ(std::this_thread::get_id() != handle_tid, true);
}

// Test when a handle thread is waiting for a condition to be ready, you can use ExecuteTask to allow the current thread
// to continue consuming tasks
TEST_F(TestSeparateThreadModel, ExecuteTask) {
  std::atomic_int counter{0};
  int handle_thread_num = TestSeparateThreadModel::separate_thread_model_->GetHandleThreadNum();
  for (int i = 0; i < handle_thread_num; i++) {
    MsgTask* req_task = trpc::object_pool::New<MsgTask>();
    req_task->task_type = trpc::runtime::kParallelTask;
    req_task->dst_thread_key = i;  // Dispatch the request to the first handle thread
    req_task->param = nullptr;
    req_task->handler = [i, handle_thread_num, &counter]() {
      HandleWorkerThread* current_thread = TestSeparateThreadModel::separate_thread_model_->GetHandleThread(i);
      while (counter.load() < handle_thread_num) {
        current_thread->ExecuteTask();  // Let the current handle thread to continue processing tasks
      }

      counter++;
    };

    TestSeparateThreadModel::separate_thread_model_->SubmitHandleTask(std::move(req_task));
  }

  for (int i = 0; i < handle_thread_num; i++) {
    MsgTask* req_task = trpc::object_pool::New<MsgTask>();
    req_task->task_type = trpc::runtime::kParallelTask;
    req_task->dst_thread_key = i;  // Dispatch the request to the i-th handle thread
    req_task->param = nullptr;
    req_task->handler = [handle_thread_num, &counter]() { counter++; };

    TestSeparateThreadModel::separate_thread_model_->SubmitHandleTask(std::move(req_task));
  }

  while (counter != 2 * handle_thread_num) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  ASSERT_EQ(counter, 2 * handle_thread_num);
}

}  // namespace testing

}  // namespace trpc

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);

  // Test the noncoroutine scheduling
  trpc::testing::TestSeparateThreadModel::SetScheduleType(trpc::testing::kNonCoroutine);
  int ret = RUN_ALL_TESTS();
  EXPECT_TRUE(ret == 0);

  // Test the taskflow scheduling
  trpc::testing::TestSeparateThreadModel::SetScheduleType(trpc::testing::kTaskflow);
  ret = RUN_ALL_TESTS();
  EXPECT_TRUE(ret == 0);
  return ret;
}
