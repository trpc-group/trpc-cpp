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
#include "trpc/runtime/threadmodel/separate/handle_worker_thread.h"
#include "trpc/runtime/threadmodel/separate/io_worker_thread.h"
#include "trpc/runtime/threadmodel/separate/separate_scheduling.h"
#include "trpc/runtime/threadmodel/thread_model.h"

namespace trpc {

/// @brief Implementation of io/handle separate thread model.
class SeparateThreadModel final : public ThreadModel {
 public:
  /// @brief configuration options of thread model instance
  struct Options {
    /// id of the thread model instance
    uint16_t group_id;

    /// name of the thread model instance
    std::string group_name;

    /// name of task scheduler been used
    std::string scheduling_name;

    /// number of io threads
    size_t io_thread_num;

    /// number of handle threads
    size_t handle_thread_num;

    /// the task queue size limit for io threads
    size_t io_task_queue_size{10240};

    /// enable asynchronous io or not
    bool enable_async_io{false};

    /// number of io_uring entries
    uint32_t io_uring_entries{1024};

    /// io_uring flags
    uint32_t io_uring_flags{0};

    /// cpu affinitys of io threads
    std::vector<uint32_t> io_cpu_affinitys;

    /// cpu affinitys of handle threads
    std::vector<uint32_t> handle_cpu_affinitys;

    /// bind cpu core strictly or not
    bool disallow_cpu_migration{false};
  };

  explicit SeparateThreadModel(Options&& options);

  ~SeparateThreadModel() override = default;

  std::string_view Type() const override { return kSeparate; }

  uint16_t GroupId() const override { return options_.group_id; }

  std::string GroupName() const override { return options_.group_name; }

  void Start() noexcept override;

  void Terminate() noexcept override;

  /// @brief submit task to handle thread
  bool SubmitHandleTask(MsgTask* handle_task) noexcept override;

  /// @brief submit task to io thread (thread-safe)
  bool SubmitIoTask(MsgTask* io_task) noexcept;

  /// @brief get all reactors contained in current thread model (thread-safe)
  /// @return std::vector<Reactor*>
  std::vector<Reactor*> GetReactors() const noexcept;

  /// @brief get one reactor instance by id (thread-safe)
  /// @param index when equal to -1, it will be obtained randomly, otherwise it will be obtained by taking the modulus
  Reactor* GetReactor(int index = -1) const noexcept;

  /// @brief get io thread by id (thread-safe)
  /// @note id must be less than number of io threads
  IoWorkerThread* GetIOThread(size_t index) const { return io_threads_[index].get(); }

  /// @brief get handle thread by id (thread-safe)
  /// @note id must be less than number of handle threads
  HandleWorkerThread* GetHandleThread(size_t index) const { return handle_threads_[index].get(); }

  /// @brief get number of io threads (thread-safe)
  int GetIoThreadNum() const { return io_threads_.size(); }

  /// @brief get number of handle threads (thread-safe)
  int GetHandleThreadNum() const { return handle_threads_.size(); }

 private:
  Options options_;

  std::unique_ptr<SeparateScheduling> handle_scheduling_{nullptr};

  std::vector<std::unique_ptr<HandleWorkerThread>> handle_threads_;

  std::vector<std::unique_ptr<IoWorkerThread>> io_threads_;
};

}  // namespace trpc
