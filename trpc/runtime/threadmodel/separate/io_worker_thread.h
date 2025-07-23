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
#include <string_view>
#include <thread>

#include "trpc/runtime/iomodel/reactor/default/reactor_impl.h"
#include "trpc/runtime/threadmodel/common/worker_thread.h"

namespace trpc {

/// @brief Implementation of io thread in separate thread model
class IoWorkerThread final : public WorkerThread {
 public:
  /// @brief configuration options of io thread
  struct Options {
    // type of the related thread model instance
    std::string_view thread_model_type;

    // name of the related thread model instance
    std::string group_name;

    // id of the related thread model instance
    uint16_t group_id;

    // logical id of the current worker thread
    uint16_t id;

    // cpu affinitys of current thread
    std::vector<uint32_t> cpu_affinitys;

    // the task queue size limit for io threads
    uint32_t io_task_queue_size{10240};

    // enable asynchronous io or not
    bool enable_async_io{false};

    // number of io_uring entries
    uint32_t io_uring_entries{1024};

    // io_uring flags
    uint32_t io_uring_flags{0};
  };

  explicit IoWorkerThread(Options&& options);

  ~IoWorkerThread() override = default;

  uint16_t Role() const override { return kIo; }

  uint16_t Id() const override { return options_.id; }

  uint16_t GroupId() const override { return options_.group_id; }

  std::string GetGroupName() const override { return options_.group_name; }

  std::string_view GetThreadModelType() const override { return options_.thread_model_type; }

  void Start() noexcept override;

  void Stop() noexcept override;

  void Join() noexcept override;

  void Destroy() noexcept override;

  /// @brief Get the reactor instance contained in the current thread
  /// @return pointer to the reactor
  Reactor* GetReactor() const noexcept { return reactor_.get(); }

 private:
  void Run() noexcept;

 private:
  Options options_;

  std::unique_ptr<std::thread> thread_;

  std::unique_ptr<ReactorImpl> reactor_;
};

}  // namespace trpc
