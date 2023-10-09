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

#include "trpc/runtime/runtime_compatible.h"

#include "trpc/runtime/threadmodel/merge/merge_thread_model.h"
#include "trpc/runtime/threadmodel/separate/separate_thread_model.h"
#include "trpc/runtime/threadmodel/thread_model_manager.h"
#include "trpc/util/log/logging.h"

namespace trpc::runtime {

//////////////////////////////////////////////

WorkerThread* GetCurrentThread() { return WorkerThread::GetCurrentWorkerThread(); }

int GetMergeThreadNum(const std::string& instance) {
  ThreadModel* thread_model = ThreadModelManager::GetInstance()->Get(instance);
  if (!thread_model) {
    TRPC_FMT_ERROR("No such thread model instance: {}", instance);
    return -1;
  }

  return static_cast<MergeThreadModel*>(thread_model)->GetThreadNum();
}

WorkerThread* GetMergeThread(uint32_t index, const std::string& instance) {
  ThreadModel* thread_model = ThreadModelManager::GetInstance()->Get(instance);
  if (!thread_model) {
    TRPC_FMT_ERROR("No such thread model instance: {}", instance);
    return nullptr;
  }

  return static_cast<MergeThreadModel*>(thread_model)->GetWorkerThreadNum(index);
}

Reactor* GetReactor(uint32_t index, const std::string& instance) {
  ThreadModel* thread_model = ThreadModelManager::GetInstance()->Get(instance);
  if (thread_model != nullptr) {
    return static_cast<MergeThreadModel*>(thread_model)->GetReactor(index);
  }

  TRPC_FMT_ERROR("No such thread model instance: {}", instance);
  return nullptr;
}

Reactor* GetThreadLocalReactor() {
  return Reactor::GetCurrentTlsReactor();
}

//////////////////////////////////////////////

int GetIOThreadNum(const std::string& instance) {
  ThreadModel* thread_model = ThreadModelManager::GetInstance()->Get(instance);
  if (!thread_model) {
    TRPC_FMT_ERROR("No such thread model instance: {}", instance);
    return -1;
  }

  return static_cast<SeparateThreadModel*>(thread_model)->GetIoThreadNum();
}

WorkerThread* GetIOThread(uint32_t index, const std::string& instance) {
  ThreadModel* thread_model = ThreadModelManager::GetInstance()->Get(instance);
  if (!thread_model) {
    TRPC_FMT_ERROR("No such thread model instance: {}", instance);
    return nullptr;
  }

  return static_cast<SeparateThreadModel*>(thread_model)->GetIOThread(index);
}

int GetHandleThreadNum(const std::string& instance) {
  ThreadModel* thread_model = ThreadModelManager::GetInstance()->Get(instance);
  if (!thread_model) {
    TRPC_FMT_ERROR("No such thread model instance: {}", instance);
    return -1;
  }

  return static_cast<SeparateThreadModel*>(thread_model)->GetHandleThreadNum();
}

WorkerThread* GetHandleThread(uint32_t index, const std::string& instance) {
  ThreadModel* thread_model = ThreadModelManager::GetInstance()->Get(instance);
  if (!thread_model) {
    TRPC_FMT_ERROR("No such thread model instance: {}", instance);
    return nullptr;
  }

  return static_cast<SeparateThreadModel*>(thread_model)->GetHandleThread(index);
}

}  // namespace trpc::runtime
