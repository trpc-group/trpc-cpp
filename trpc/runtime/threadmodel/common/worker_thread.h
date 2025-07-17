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
#include <string_view>
#include <thread>

namespace trpc {

/// @brief role of worker thread
constexpr uint16_t kIo = 0x01;
constexpr uint16_t kHandle = 0x10;
constexpr uint16_t kIoAndHandle = kIo | kHandle;

/// @brief Base class for worker thread
class WorkerThread {
 public:
  virtual ~WorkerThread() = default;

  /// @brief Get the worker thread pointer that is private to the current thread
  static WorkerThread* GetCurrentWorkerThread() { return current_worker_thread; }

  /// @brief Set the worker thread pointer that is private to the current thread
  static void SetCurrentWorkerThread(WorkerThread* current) { current_worker_thread = current; }

  /// @brief Set the current thread name displayed in the top command
  static void SetCurrentThreadName(WorkerThread* current);

  /// @brief Get unique id of current thread, it consists of the group id and the thread logical id
  uint32_t GetUniqueId() const { return (static_cast<uint32_t>(GroupId()) << 16) | Id(); }

  /// @brief Get the role of current thread
  virtual uint16_t Role() const = 0;

  /// @brief Get the logical id of current worker thread
  virtual uint16_t Id() const = 0;

  /// @brief Get the group id of the thread model that the current thread belongs to
  virtual uint16_t GroupId() const = 0;

  /// @brief Get the group name of the thread model that the current thread belongs to
  virtual std::string GetGroupName() const = 0;

  /// @brief Get the type of the thread model that the current thread belongs to
  virtual std::string_view GetThreadModelType() const = 0;

  /// @brief Start worker thread
  virtual void Start() noexcept = 0;

  /// @brief Stop worker thread
  virtual void Stop() noexcept = 0;

  /// @brief Wail until this worker quits
  virtual void Join() noexcept = 0;

  /// @brief Destroy the internal resources of the worker thread
  virtual void Destroy() noexcept = 0;

  /// @brief execute tasks in the queue, currently used to integrate with the taskflow ecosystem.
  ///        so that executing dynamic graphs does not block the current thread.
  virtual void ExecuteTask() noexcept {}

 private:
  static inline thread_local WorkerThread* current_worker_thread = nullptr;
};

}  // namespace trpc
