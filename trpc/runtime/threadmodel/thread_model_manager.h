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

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>

#include "trpc/runtime/threadmodel/thread_model.h"

namespace trpc {

/// @brief Management class for thread model
class ThreadModelManager {
 public:
  static ThreadModelManager* GetInstance() {
    static ThreadModelManager instance;
    return &instance;
  }

  /// @brief Register an instance of threadmodel
  /// @note  Query by the type and instance(group) name of the thread_model as the key
  ///        if there is no conflict, the registration is successful, otherwise it fails
  bool Register(const std::shared_ptr<ThreadModel>& thread_model);

  /// @brief Get the specific instance of threadmodel by the instance name
  /// @param name The instance(group) name of the thread model
  /// @return Returns a pointer to the thread model instance, the acquirer does not own the pointer
  ThreadModel* Get(const std::string& name);

  /// @brief Deprecated. Get the specific instance of threadmodel
  ///        according to the type and instance name of the thread model
  /// @param type The type of threading model
  /// @param name The instance(group) name of the thread model
  /// @return Returns a pointer to the thread model instance, the acquirer does not own the pointer
  [[deprecated("Use Get(const std::string& name) instead")]] ThreadModel* Get(std::string_view type,
                                                                              const std::string& name);

  /// @brief Del the specific instance of threadmodel by the instance name
  /// @param name The instance(group) name of the thread model
  void Del(const std::string& name);

  /// @brief Deprecated. Del the specific instance of threadmodel by type and name
  /// @param type The type of threading model
  /// @param name The instance(group) name of the thread model
  [[deprecated("Use Del(const std::string& name) instead")]] void Del(std::string_view type, const std::string& name);

  /// @brief Generate a unique id for each thread model instance
  uint16_t GenGroupId() { return gen_group_id_.fetch_add(1, std::memory_order_relaxed); }

  /// @brief Get the size of thread model instance
  std::size_t Size() const { return threadmodel_instances_.size(); }

 private:
  ThreadModelManager();
  ~ThreadModelManager();
  ThreadModelManager(const ThreadModelManager&) = delete;
  ThreadModelManager& operator=(const ThreadModelManager&) = delete;

  // a collection of instances of the same threading model type
  // key is the instance(group) name of the thread model
  using ThreadModelInstanceMap = std::unordered_map<std::string, std::shared_ptr<ThreadModel>>;

  // group id generator for threading model instances, start from 1
  std::atomic<uint16_t> gen_group_id_{1};

  // collection of thread model instance
  // key is the type of thread model
  ThreadModelInstanceMap threadmodel_instances_;
};

}  // namespace trpc
