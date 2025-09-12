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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. taskflow
// Copyright (C) 2018-2022 Dr. Tsung-Wei Huang
// taskflow is licensed under the MIT License.
//

#include "trpc/util/thread/mq_thread_pool.h"

#include "trpc/util/bind_core_manager.h"
#include "trpc/util/log/logging.h"

namespace trpc {

MQThreadPool::MQThreadPool(ThreadPoolOption&& thread_pool_option)
    : thread_pool_option_(std::move(thread_pool_option)),
      workers_{thread_pool_option_.thread_num},
      notifier_{thread_pool_option_.thread_num} {
  TRPC_ASSERT(thread_pool_option_.thread_num > 0);
  TRPC_ASSERT(thread_pool_option_.task_queue_size > 0);
}

void MQThreadPool::SetThreadInitFunc(Function<void()>&& func) {
  thread_init_func_ = std::move(func);
}

bool MQThreadPool::Start() {
  TRPC_ASSERT(global_task_queue_.Init(thread_pool_option_.task_queue_size) == true);

  Spawn(thread_pool_option_.thread_num);

  return true;
}

bool MQThreadPool::AddTask(Function<void()>&& job) {
  if (IsStop()) {
    return false;
  }

  auto worker = ThisWorker().worker;
  // The current thread adds tasks to its own queue
  if (worker != nullptr && worker->mq_thread_pool_ == this) {
    worker->task_queue_.Push(new MQThreadPoolTask(std::move(job)));
    return true;
  }

  auto task = new MQThreadPoolTask(std::move(job));
  // Other threads add tasks to the global queue
  if (global_task_queue_.Push(task)) {
    // notify consume
    notifier_.Notify(false);
    return true;
  } else {
    delete task;
  }

  return false;
}

void MQThreadPool::Stop() {
  done_ = true;
  notifier_.Notify(true);

  for (auto& t : threads_) {
    t.join();
  }
}

bool MQThreadPool::IsStop() { return done_; }

void MQThreadPool::Spawn(size_t thread_num) {
  for (size_t id = 0; id < thread_num; ++id) {
    workers_[id].id_ = id;
    workers_[id].vtm_ = id;
    workers_[id].mq_thread_pool_ = this;
    workers_[id].waiter_ = &notifier_.GetWaiter(id);

    threads_.emplace_back(
        [this](MQThreadPoolWorker& worker) -> void {
          if (this->thread_pool_option_.bind_core) {
            BindCoreManager::BindCore();
          }
          if (thread_init_func_ != nullptr) {
            thread_init_func_();
          }
          ThisWorker().worker = &worker;

          MQThreadPoolTask* task{nullptr};
          while (true) {
            ExecuteTask(worker, task);
            if (WaitForTask(worker, task) == false) {
              break;
            }
          }
        },
        std::ref(workers_[id]));
  }
}

// The following source codes are from taskflow.
// Copied and modified from
// https://github.com/taskflow/taskflow/blob/v3.5.0/taskflow/core/executor.hpp.

void MQThreadPool::ExecuteTask(MQThreadPoolWorker& worker, MQThreadPoolTask*& task) {
  if (task) {
    // If the current threads are all inactive,
    // then try to activate to handle possible incoming tasks
    if (num_actives_.fetch_add(1) == 0 && num_thieves_ == 0) {
      notifier_.Notify(false);
    }

    // Execute the tasks in its own queue until the end,
    // then exit and steal tasks from other queues
    while (task) {
      Invoke(worker, task);
      task = worker.task_queue_.Pop();
    }
    --num_actives_;
  }
}

bool MQThreadPool::WaitForTask(MQThreadPoolWorker& worker, MQThreadPoolTask*& task) {
  while (true) {
    ++num_thieves_;

    while (true) {
      StealTask(worker, task);

      if (task) {
        if (num_thieves_.fetch_sub(1) == 1) {  // try to activate a thread
          notifier_.Notify(false);
        }
        return true;
      }
      // There is no task that can be running, and a second judgment is made
      notifier_.PrepareWait(worker.waiter_);
      if (global_task_queue_.Size() != 0) {
        notifier_.CancelWait(worker.waiter_);
        if (global_task_queue_.Pop(task)) {
          if (num_thieves_.fetch_sub(1) == 1) {
            notifier_.Notify(false);
          }
          return true;
        } else {  // If it has been stolen by other threads, continue to check
          worker.vtm_ = worker.id_;
          continue;
        }
      } else {
        break;
      }
    }

    if (done_) {
      notifier_.CancelWait(worker.waiter_);
      notifier_.Notify(true);
      --num_thieves_;
      return false;
    }

    if (num_thieves_.fetch_sub(1) == 1) {
      if (num_actives_) {
        notifier_.CancelWait(worker.waiter_);
        continue;
      }
      for (auto& w : workers_) {
        if (!w.task_queue_.Empty()) {
          worker.vtm_ = w.id_;
          notifier_.CancelWait(worker.waiter_);
          continue;
        }
      }
    }
    // If the above checks fail, then it must be in a waiting state
    notifier_.CommitWait(worker.waiter_);
    break;
  }

  return true;
}

void MQThreadPool::StealTask(MQThreadPoolWorker& worker, MQThreadPoolTask*& task) {
  size_t num_steals = 0;
  size_t num_yields = 0;
  size_t max_steals = ((workers_.size() + 1) << 1);

  std::uniform_int_distribution<size_t> rdvtm(0, workers_.size() - 1);

  do {
    if (worker.id_ == worker.vtm_) {
      global_task_queue_.Pop(task);
    } else {
      task = workers_[worker.vtm_].task_queue_.Steal();
    }

    if (task) {
      break;
    }

    if (num_steals++ > max_steals) {
      std::this_thread::yield();
      if (num_yields++ > 100) {
        break;
      }
    }

    worker.vtm_ = rdvtm(worker.rdgen_);
  } while (!done_);
}

// End of source codes that are from taskflow.

void MQThreadPool::Invoke(MQThreadPoolWorker& worker, MQThreadPoolTask* task) {
  if (task) {
    task->task_func_();
    delete task;
    task = nullptr;
  }
}

}  // namespace trpc
