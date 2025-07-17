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

#include "trpc/runtime/threadmodel/merge/merge_thread_model.h"

#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/runtime/iomodel/reactor/default/reactor_impl.h"
#include "trpc/runtime/threadmodel/common/msg_task.h"
#include "trpc/util/object_pool/object_pool_ptr.h"
#include "trpc/util/time.h"

namespace trpc {

namespace testing {

class TestMergeThreadModel : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    merge_thread_model_ = CreateMergeThreadModel();
    merge_thread_model_->Start();
  }

  static MergeThreadModel* CreateMergeThreadModel() {
    MergeThreadModel::Options thread_model_options;
    thread_model_options.group_id = 0;
    thread_model_options.group_name = "group_name";
    thread_model_options.worker_thread_num = 2;
    thread_model_options.max_task_queue_size = 10;
    thread_model_options.cpu_affinitys = {1, 0};

    return new MergeThreadModel(std::move(thread_model_options));
  }

  static void TearDownTestCase() {
    merge_thread_model_->Terminate();
    delete merge_thread_model_;
    merge_thread_model_ = nullptr;
  }

 protected:
  static MergeThreadModel* merge_thread_model_;
};

MergeThreadModel* TestMergeThreadModel::merge_thread_model_ = nullptr;

// Test submit IO tasks on non-framework threads, and randomly assign them to the framework's IO threads for execution
TEST_F(TestMergeThreadModel, SubmitIoTask1OnMerge) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id tid;

  MsgTask* task = trpc::object_pool::New<MsgTask>();
  task->task_type = trpc::runtime::kResponseMsg;
  task->param = nullptr;
  task->handler = [&ret, &mutex, &cond, &tid]() {
    tid = std::this_thread::get_id();
    std::unique_lock<std::mutex> lock(mutex);
    ret = 1;
    cond.notify_all();
  };
  task->group_id = merge_thread_model_->GroupId();

  merge_thread_model_->SubmitIoTask(task);

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  ASSERT_EQ(ret, 1);
  ASSERT_EQ(std::this_thread::get_id() != tid, true);
}

// Test submit IO tasks on non-framework threads and specify a specific IO thread for execution
TEST_F(TestMergeThreadModel, SubmitIoTask2OnMerge) {
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

  merge_thread_model_->SubmitIoTask(std::move(task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  ASSERT_EQ(ret, 1);
  ASSERT_EQ(std::this_thread::get_id() != tid, true);
  ASSERT_EQ(thread_index, 1);
}

// Submit IO tasks on framework IO threads and run the task on the same IO thread
TEST_F(TestMergeThreadModel, SubmitIoTask3OnMerge) {
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

    TestMergeThreadModel::merge_thread_model_->SubmitIoTask(task);
  };

  TestMergeThreadModel::merge_thread_model_->GetReactor(thread_index)->SubmitTask(std::move(reactor_task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  ASSERT_EQ(ret, 1);
  ASSERT_EQ(std::this_thread::get_id() != tid, true);
  ASSERT_EQ(thread_index, 0);
  ASSERT_EQ(task_execute_thread_index, thread_index);
}

// Test submit IO tasks on framework IO threads and specify to run the task on the thread that submitted it
TEST_F(TestMergeThreadModel, SubmitIoTask4OnMerge) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id tid;
  int thread_index = 0;
  int task_execute_thread_index = -1;

  Reactor::Task reactor_task = [&ret, &mutex, &cond, &tid, &thread_index, &task_execute_thread_index] {
    task_execute_thread_index = WorkerThread::GetCurrentWorkerThread()->Id();
    std::cout << "4task_execute_thread_index:" << task_execute_thread_index << std::endl;
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

    TestMergeThreadModel::merge_thread_model_->SubmitIoTask(task);
  };

  TestMergeThreadModel::merge_thread_model_->GetReactor(thread_index)->SubmitTask(std::move(reactor_task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  ASSERT_EQ(ret, 1);
  ASSERT_EQ(std::this_thread::get_id() != tid, true);
  ASSERT_EQ(thread_index, 0);
  ASSERT_EQ(task_execute_thread_index, thread_index);
}

// Test submit IO tasks on framework IO threads and specify to run the task on other IO threads
TEST_F(TestMergeThreadModel, SubmitIoTask5OnMerge) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id tid;
  int thread_index = 0;
  int task_execute_thread_index = -1;

  Reactor::Task reactor_task = [&ret, &mutex, &cond, &tid, &thread_index, &task_execute_thread_index] {
    task_execute_thread_index = WorkerThread::GetCurrentWorkerThread()->Id();
    std::cout << "5task_execute_thread_index:" << task_execute_thread_index << std::endl;
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

    TestMergeThreadModel::merge_thread_model_->SubmitIoTask(task);
  };

  TestMergeThreadModel::merge_thread_model_->GetReactor(thread_index)->SubmitTask(std::move(reactor_task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  ASSERT_EQ(ret, 1);
  ASSERT_EQ(std::this_thread::get_id() != tid, true);
  ASSERT_EQ(thread_index, 1);
  ASSERT_EQ(task_execute_thread_index, 0);
  ASSERT_EQ(task_execute_thread_index != thread_index, true);
}

// Test submit handle tasks to the framework's IO threads for execution
TEST_F(TestMergeThreadModel, SubmitHandleTask1OnMerge) {
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

  merge_thread_model_->SubmitHandleTask(std::move(task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  ASSERT_EQ(ret, 1);
  ASSERT_EQ(std::this_thread::get_id() != tid, true);
}

// Test submit handle tasks on framework IO threads and run the task on the same IO thread
TEST_F(TestMergeThreadModel, SubmitHandleTask2OnMerge) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id tid;
  int thread_index = 0;
  int task_execute_thread_index = -1;

  Reactor::Task reactor_task = [&ret, &mutex, &cond, &tid, &thread_index, &task_execute_thread_index] {
    task_execute_thread_index = WorkerThread::GetCurrentWorkerThread()->Id();

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

    TestMergeThreadModel::merge_thread_model_->SubmitHandleTask(std::move(task));
  };

  TestMergeThreadModel::merge_thread_model_->GetReactor(thread_index)->SubmitTask(std::move(reactor_task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  ASSERT_EQ(ret, 1);
  ASSERT_EQ(std::this_thread::get_id() != tid, true);
  ASSERT_EQ(thread_index, 0);
  ASSERT_EQ(task_execute_thread_index, thread_index);
}

// Test submit handle tasks on framework IO threads and specify to run the task on a specific IO thread
TEST_F(TestMergeThreadModel, SubmitHandleTask3OnMerge) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id tid;
  int thread_index = 0;
  int task_execute_thread_index = -1;

  Reactor::Task reactor_task = [&ret, &mutex, &cond, &tid, &thread_index, &task_execute_thread_index] {
    task_execute_thread_index = WorkerThread::GetCurrentWorkerThread()->Id();

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

    TestMergeThreadModel::merge_thread_model_->SubmitHandleTask(std::move(task));
  };

  TestMergeThreadModel::merge_thread_model_->GetReactor(0)->SubmitTask(std::move(reactor_task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  ASSERT_EQ(ret, 1);
  ASSERT_EQ(std::this_thread::get_id() != tid, true);
  ASSERT_EQ(thread_index, 1);
  ASSERT_EQ(task_execute_thread_index != thread_index, true);
}

// Test submit handle tasks on framework IO threads and run the task on the same IO thread
TEST_F(TestMergeThreadModel, SubmitHandleTask4OnMerge) {
  int ret = 0;
  std::mutex mutex;
  std::condition_variable cond;
  std::thread::id tid;
  int thread_index = 0;
  int task_execute_thread_index = -1;

  Reactor::Task reactor_task = [&ret, &mutex, &cond, &tid, &thread_index, &task_execute_thread_index] {
    task_execute_thread_index = WorkerThread::GetCurrentWorkerThread()->Id();

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
    task->dst_thread_key = 0;

    TestMergeThreadModel::merge_thread_model_->SubmitHandleTask(std::move(task));
  };

  TestMergeThreadModel::merge_thread_model_->GetReactor(0)->SubmitTask(std::move(reactor_task));

  std::unique_lock<std::mutex> lock(mutex);
  while (!ret) cond.wait(lock);

  ASSERT_EQ(ret, 1);
  ASSERT_EQ(std::this_thread::get_id() != tid, true);
  ASSERT_EQ(thread_index, 0);
  ASSERT_EQ(task_execute_thread_index, thread_index);
}

// Test when an executing thread waits for a condition to be ready, you can use ExecuteTask to allow the current thread
// to continue consuming tasks
TEST_F(TestMergeThreadModel, ExecuteTask) {
  std::atomic_int counter{0};

  MsgTask* wait_task = trpc::object_pool::New<MsgTask>();
  wait_task->task_type = trpc::runtime::kParallelTask;
  wait_task->dst_thread_key = 0;  // Dispatch the request to the first worker thread
  wait_task->param = nullptr;
  wait_task->handler = [&counter]() {
    std::cout << "handler1" << std::endl;
    MergeWorkerThread* current_thread = TestMergeThreadModel::merge_thread_model_->GetWorkerThreadNum(0);
    while (counter.load() < 1) {
      current_thread->ExecuteTask();  // Let the current handle thread to continue processing tasks
    }

    counter++;
  };

  TestMergeThreadModel::merge_thread_model_->SubmitHandleTask(std::move(wait_task));

  // Set the counter to 1 to make the previous task exit the loop
  MsgTask* task = trpc::object_pool::New<MsgTask>();
  task->task_type = trpc::runtime::kParallelTask;
  task->dst_thread_key = 0;  // Dispatch the request to the first worker thread
  task->param = nullptr;
  task->handler = [&counter]() {
    std::cout << "handler2" << std::endl;
    counter++;
  };

  TestMergeThreadModel::merge_thread_model_->SubmitHandleTask(task);

  while (counter != 2) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  ASSERT_EQ(counter, 2);
}

}  // namespace testing

}  // namespace trpc
