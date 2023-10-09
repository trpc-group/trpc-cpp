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

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <thread>

#include "trpc/runtime/threadmodel/separate/separate_scheduling.h"
#include "trpc/runtime/threadmodel/common/worker_thread.h"

namespace trpc {

/// @brief Implementation of handle thread in separate thread model
class HandleWorkerThread final : public WorkerThread {
 public:
  /// @brief configuration options of handle thread
  struct Options {
    // type of the related thread model instance
    std::string_view thread_model_type;

    // name of the related thread model instance
    std::string group_name;

    // id of the related thread model instance
    uint16_t group_id;

    // logical id of the current worker thread
    uint16_t id;

    // scheduling used by the current worker thread
    SeparateScheduling* scheduling;

    // cpu affinitys of current thread
    std::vector<uint32_t> cpu_affinitys;
  };

  explicit HandleWorkerThread(Options&& options);

  ~HandleWorkerThread() override = default;

  uint16_t Role() const override { return kHandle; }

  uint16_t Id() const override { return options_.id; }

  uint16_t GroupId() const override { return options_.group_id; }

  std::string GetGroupName() const override { return options_.group_name; }

  std::string_view GetThreadModelType() const override { return options_.thread_model_type; }

  SeparateScheduling* GetSeparateScheduling() const { return options_.scheduling; }

  void Start() noexcept override;

  void Stop() noexcept override;

  void Join() noexcept override;

  void Destroy() noexcept override;

  void ExecuteTask() noexcept override;

 private:
  void Run() noexcept;

 private:
  Options options_;

  std::unique_ptr<std::thread> thread_;
};

}  // namespace trpc
