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

#include "trpc/runtime/common/periphery_task_scheduler.h"

#include <chrono>

#include "trpc/common/config/trpc_config.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/thread/thread_helper.h"

using namespace std::literals;

namespace trpc {

PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::~PeripheryTaskSchedulerImpl() {
  Stop();
  Join();
}

void PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::Init(size_t thread_num) { thread_num_ = thread_num; }

void PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::Start() {
  if (thread_num_ > 0) {
    exited_.store(false, std::memory_order_relaxed);
  }
  for (unsigned i = 0; i < thread_num_; ++i) {
    workers_.push_back(std::thread(std::bind(&PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::Schedule, this)));
  }
}

void PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::Stop() {
  exited_.store(true, std::memory_order_relaxed);
  std::scoped_lock _(lock_);
  cv_.notify_all();
}

void PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::Join() {
  for (unsigned i = 0; i < thread_num_; ++i) {
    if (workers_[i].joinable()) {
      workers_[i].join();
    }
  }
  workers_.clear();

  // clear the resources when all worker threads were exited
  if (clean_cb_) {
    clean_cb_();
    clean_cb_ = nullptr;
  }

  while (!tasks_.empty()) {
    tasks_.pop();
  }
}

std::uint64_t PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::SubmitTaskImpl(Function<void()>&& task,
                                                                                 const std::string& name) {
  if (TRPC_UNLIKELY(exited_.load(std::memory_order_relaxed))) {
    TRPC_FMT_ERROR("PeripheryTaskScheduler is already exited.");
    return 0;
  }
  return CreateAndSubmitTask(
      std::move(task), []() -> std::uint64_t { return 0; }, false, name);
}

std::uint64_t PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::SubmitPeriodicalTaskImpl(Function<void()>&& task,
                                                                                           std::uint64_t interval,
                                                                                           const std::string& name) {
  if (TRPC_UNLIKELY(exited_.load(std::memory_order_relaxed))) {
    TRPC_FMT_ERROR("PeripheryTaskScheduler is already exited.");
    return 0;
  }
  return CreateAndSubmitTask(
      std::move(task), [interval]() -> std::uint64_t { return interval; }, true, name);
}

std::uint64_t PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::SubmitPeriodicalTaskImpl(
    Function<void()>&& task, Function<std::uint64_t()>&& interval_gen_func, const std::string& name) {
  if (TRPC_UNLIKELY(exited_.load(std::memory_order_relaxed))) {
    TRPC_FMT_ERROR("PeripheryTaskScheduler is already exited.");
    return 0;
  }
  return CreateAndSubmitTask(std::move(task), std::move(interval_gen_func), true, name);
}

PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::TaskPtr
PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::GetTaskPtr(std::uint64_t task_id) {
  if (task_id == 0) {
    return nullptr;
  }
  TaskPtr task_ptr(adopt_ptr, reinterpret_cast<Task*>(task_id));
  if (TRPC_LIKELY(task_ptr->UnsafeRefCount() > 1)) {
    return task_ptr;
  }
  TRPC_FMT_ERROR("Task with id {} not exist", task_id);
  return nullptr;
}

bool PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::StopAndDestroyTask(std::uint64_t task_id, bool is_destroy) {
  if (TRPC_UNLIKELY(exited_.load(std::memory_order_relaxed))) {
    TRPC_FMT_ERROR("PeripheryTaskScheduler is already exited.");
    return false;
  }

  TaskPtr task_ptr = GetTaskPtr(task_id);
  if (task_ptr.Get() == nullptr) {
    return false;
  }

  std::scoped_lock _(task_ptr->lock);
  task_ptr->cancelled = true;
  if (!is_destroy) {
    task_ptr->Ref();
  }
  return true;
}

bool PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::RemoveTaskImpl(uint64_t task_id) {
  return StopAndDestroyTask(task_id, true);
}

bool PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::StopTaskImpl(uint64_t task_id) {
  return StopAndDestroyTask(task_id, false);
}

bool PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::JoinTaskImpl(uint64_t task_id) {
  if (TRPC_UNLIKELY(exited_.load(std::memory_order_relaxed))) {
    TRPC_FMT_ERROR("PeripheryTaskScheduler is already exited.");
    return false;
  }

  TaskPtr task_ptr = GetTaskPtr(task_id);
  if (task_ptr.Get() == nullptr) {
    return false;
  }

  std::unique_lock lk(task_ptr->lock);
  // Check whether the task has stopped every 100ms, using 'done' as the condition to prevent the problem of being
  // unable to terminate before executing JoinTask due to being notified.
  while (!task_ptr->stop_cv.wait_for(lk, 100ms, [task_ptr, this] {
    return this->exited_.load(std::memory_order_relaxed) || task_ptr->done ||
           (task_ptr->cancelled && !task_ptr->running);
  })) { /* do nothing */
  }
  return true;
}

void PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::Schedule() {
  SetCurrentThreadName("periphery_task_scheduler");
  while (!exited_.load(std::memory_order_relaxed)) {
    std::unique_lock lk(lock_);
    auto pred = [&] {
      return (!tasks_.empty() && tasks_.top()->expires_at <= std::chrono::steady_clock::now()) ||
             exited_.load(std::memory_order_relaxed);
    };
    auto next = tasks_.empty() ? std::chrono::steady_clock::now() + 100s : tasks_.top()->expires_at;
    cv_.wait_until(lk, next, pred);

    if (exited_.load(std::memory_order_relaxed)) {
      break;
    }
    if (!pred()) {
      continue;
    }

    // get and execute task
    auto task = tasks_.top();
    tasks_.pop();

    lk.unlock();
    TaskProc(std::move(task));
  }
}

void PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::TaskProc(TaskPtr&& task_ptr) {
  {
    std::scoped_lock _(task_ptr->lock);
    if (task_ptr->cancelled) {
      task_ptr->done = true;
      task_ptr->stop_cv.notify_all();
      return;
    }
    task_ptr->running = true;
  }

  task_ptr->cb();

  {
    std::scoped_lock _(task_ptr->lock);
    task_ptr->running = false;
    if (!task_ptr->periodic || task_ptr->cancelled) {
      task_ptr->done = true;
      task_ptr->stop_cv.notify_all();
      return;
    }
    task_ptr->expires_at += std::chrono::milliseconds(task_ptr->interval_gen_func());
  }

  std::scoped_lock _(lock_);
  tasks_.push(std::move(task_ptr));
  cv_.notify_all();
}

std::uint64_t PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::CreateAndSubmitTask(
    Function<void()>&& cb, Function<std::uint64_t()>&& interval_gen_func, bool periodic, const std::string& name) {
  auto task_ptr = trpc::MakeRefCounted<Task>();
  task_ptr->name = name;
  task_ptr->expires_at = std::chrono::steady_clock::now();
  task_ptr->periodic = periodic;
  task_ptr->cb = std::move(cb);
  task_ptr->interval_gen_func = std::move(interval_gen_func);
  task_ptr->Ref();

  uint64_t task_id = reinterpret_cast<std::uint64_t>(task_ptr.Get());

  std::scoped_lock _(lock_);
  tasks_.push(std::move(task_ptr));
  cv_.notify_all();
  return task_id;
}

bool PeripheryTaskScheduler::PeripheryTaskSchedulerImpl::TaskPtrComp::operator()(const TaskPtr& p1,
                                                                                 const TaskPtr& p2) const {
  return p1->expires_at > p2->expires_at;
}

PeripheryTaskScheduler::~PeripheryTaskScheduler() {
  Stop();
  Join();
}

PeripheryTaskScheduler* PeripheryTaskScheduler::GetInstance() {
  static PeripheryTaskScheduler instance;
  return &instance;
}

bool PeripheryTaskScheduler::Init() {
  if (started_) {
    return true;
  }

  scheduler_ = std::make_unique<PeripheryTaskSchedulerImpl>();
  scheduler_->Init(TrpcConfig::GetInstance()->GetGlobalConfig().periphery_task_scheduler_thread_num);

  inner_scheduler_ = std::make_unique<PeripheryTaskSchedulerImpl>();
  inner_scheduler_->Init(TrpcConfig::GetInstance()->GetGlobalConfig().inner_periphery_task_scheduler_thread_num);

  return true;
}

void PeripheryTaskScheduler::Start() {
  if (started_) {
    return;
  }

  scheduler_->Start();
  inner_scheduler_->Start();
  started_ = true;
}

void PeripheryTaskScheduler::Stop() {
  if (!started_) {
    return;
  }

  scheduler_->Stop();
  inner_scheduler_->Stop();
}

void PeripheryTaskScheduler::Join() {
  if (!started_) {
    return;
  }

  scheduler_->Join();
  inner_scheduler_->Join();

  scheduler_ = nullptr;
  inner_scheduler_ = nullptr;
  started_ = false;
}

std::uint64_t PeripheryTaskScheduler::SubmitTask(Function<void()>&& task, const std::string& name) {
  if (!scheduler_) {
    return 0;
  }
  return scheduler_->SubmitTaskImpl(std::move(task), name);
}

std::uint64_t PeripheryTaskScheduler::SubmitPeriodicalTask(Function<void()>&& task, std::uint64_t interval,
                                                           const std::string& name) {
  if (!scheduler_) {
    return 0;
  }
  return scheduler_->SubmitPeriodicalTaskImpl(std::move(task), interval, name);
}

std::uint64_t PeripheryTaskScheduler::SubmitPeriodicalTask(Function<void()>&& task,
                                                           Function<std::uint64_t()>&& interval_gen_func,
                                                           const std::string& name) {
  if (!scheduler_) {
    return 0;
  }
  return scheduler_->SubmitPeriodicalTaskImpl(std::move(task), std::move(interval_gen_func), name);
}

bool PeripheryTaskScheduler::RemoveTask(uint64_t task_id) {
  if (!scheduler_) {
    return false;
  }
  return scheduler_->RemoveTaskImpl(task_id);
}

bool PeripheryTaskScheduler::StopTask(uint64_t task_id) {
  if (!scheduler_) {
    return false;
  }
  return scheduler_->StopTaskImpl(task_id);
}

bool PeripheryTaskScheduler::JoinTask(uint64_t task_id) {
  if (!scheduler_) {
    return false;
  }
  return scheduler_->JoinTaskImpl(task_id);
}

std::uint64_t PeripheryTaskScheduler::SubmitInnerTask(Function<void()>&& task, const std::string& name) {
  if (!inner_scheduler_) {
    return 0;
  }
  return inner_scheduler_->SubmitTaskImpl(std::move(task), name);
}

std::uint64_t PeripheryTaskScheduler::SubmitInnerPeriodicalTask(Function<void()>&& task, std::uint64_t interval,
                                                                const std::string& name) {
  if (!inner_scheduler_) {
    return 0;
  }
  return inner_scheduler_->SubmitPeriodicalTaskImpl(std::move(task), interval, name);
}

std::uint64_t PeripheryTaskScheduler::SubmitInnerPeriodicalTask(Function<void()>&& task,
                                                                Function<std::uint64_t()> interval_gen_func,
                                                                const std::string& name) {
  if (!inner_scheduler_) {
    return 0;
  }
  return inner_scheduler_->SubmitPeriodicalTaskImpl(std::move(task), std::move(interval_gen_func), name);
}

bool PeripheryTaskScheduler::RemoveInnerTask(uint64_t task_id) {
  if (!inner_scheduler_) {
    return false;
  }
  return inner_scheduler_->RemoveTaskImpl(task_id);
}

bool PeripheryTaskScheduler::StopInnerTask(uint64_t task_id) {
  if (!inner_scheduler_) {
    return false;
  }
  return inner_scheduler_->StopTaskImpl(task_id);
}

bool PeripheryTaskScheduler::JoinInnerTask(uint64_t task_id) {
  if (!inner_scheduler_) {
    return false;
  }
  return inner_scheduler_->JoinTaskImpl(task_id);
}

}  // namespace trpc
