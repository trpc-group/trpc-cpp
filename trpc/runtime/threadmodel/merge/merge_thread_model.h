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

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "trpc/runtime/iomodel/reactor/reactor.h"
#include "trpc/runtime/threadmodel/common/msg_task.h"
#include "trpc/runtime/threadmodel/thread_model.h"
#include "trpc/runtime/threadmodel/merge/merge_worker_thread.h"

namespace trpc {

/// @brief Implementation of io/handle merge thread model.
class MergeThreadModel final : public ThreadModel {
 public:
  /// @brief configuration options of thread model instance
  struct Options {
    /// id of the thread model instance
    uint16_t group_id;

    /// name of the thread model instance
    std::string group_name;

    /// number of worker threads
    size_t worker_thread_num;

    /// cpu affinitys of worker threads
    std::vector<uint32_t> cpu_affinitys;

    /// the task queue size limit for worker thread
    uint32_t max_task_queue_size{10240};

    /// enable asynchronous io or not
    bool enable_async_io{false};

    /// number of io_uring entries
    uint32_t io_uring_entries{1024};

    /// io_uring flags
    uint32_t io_uring_flags{0};

    /// bind cpu core strictly or not
    bool disallow_cpu_migration{false};
  };

  explicit MergeThreadModel(Options&& options);

  ~MergeThreadModel() override = default;

  std::string_view Type() const override { return kMerge; }

  uint16_t GroupId() const override { return options_.group_id; }

  std::string GroupName() const override { return options_.group_name; }

  void Start() noexcept override;

  void Terminate() noexcept override;

  /// @brief Submit io task (thread-safe)
  bool SubmitHandleTask(MsgTask* handle_task) noexcept override;

  /// @brief Submit io task (thread-safe)
  bool SubmitIoTask(MsgTask* io_task) noexcept;

  /// @brief @brief Get all reactors contained in current thread model (thread-safe)
  std::vector<Reactor*> GetReactors() const noexcept;

  /// @brief Get one reactor instance by id (thread-safe)
  /// @param index when equal to -1, it will be obtained randomly, otherwise it will be obtained by taking the modulus
  Reactor* GetReactor(int index = -1) const noexcept;

  /// @brief Get io thread by id (thread-safe)
  /// @note id must be less than number of io threads
  MergeWorkerThread* GetWorkerThreadNum(size_t index) const { return worker_threads_[index].get(); }

  /// @brief Get number of worker threads
  int GetThreadNum() const { return worker_threads_.size(); }

 private:
  bool SubmitMsgTask(MsgTask* task) noexcept;

 private:
  Options options_;

  std::vector<std::unique_ptr<MergeWorkerThread>> worker_threads_;
};

}  // namespace trpc
