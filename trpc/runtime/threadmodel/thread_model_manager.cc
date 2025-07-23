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

#include "trpc/runtime/threadmodel/thread_model_manager.h"

#include "trpc/util/log/logging.h"

namespace trpc {

ThreadModelManager::ThreadModelManager()
    : gen_group_id_(1) {
}

ThreadModelManager::~ThreadModelManager() {
  threadmodel_instances_.clear();
}

bool ThreadModelManager::Register(const std::shared_ptr<ThreadModel>& thread_model) {
  TRPC_FMT_DEBUG("{} : {} registered.", thread_model->Type(), thread_model->GroupName());
  auto it = threadmodel_instances_.find(thread_model->GroupName());
  if (it != threadmodel_instances_.end()) {
    TRPC_FMT_ERROR("{} : {} has registered already.", thread_model->Type(), thread_model->GroupName());
    return false;
  }

  threadmodel_instances_[thread_model->GroupName()] = thread_model;
  return true;
}

ThreadModel* ThreadModelManager::Get(const std::string& name) {
  auto it = threadmodel_instances_.find(name);
  if (it != threadmodel_instances_.end()) {
    return it->second.get();
  }

  return nullptr;
}

ThreadModel* ThreadModelManager::Get(std::string_view type, const std::string& name) {
  (void)type;
  return Get(name);
}

void ThreadModelManager::Del(const std::string& name) {
  auto it = threadmodel_instances_.find(name);
  if (it != threadmodel_instances_.end()) {
    threadmodel_instances_.erase(it);
  }
}

void ThreadModelManager::Del(std::string_view type, const std::string& name) {
  (void)type;
  return Del(name);
}

}  // namespace trpc
