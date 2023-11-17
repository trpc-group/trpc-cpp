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

#include "trpc/runtime/merge_runtime.h"

#include "trpc/common/config/global_conf.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/runtime/threadmodel/merge/merge_thread_model.h"
#include "trpc/runtime/threadmodel/thread_model_manager.h"
#include "trpc/util/check.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/random.h"
#include "trpc/util/thread/thread_helper.h"

namespace trpc::merge {

// business_threadmodels has not owned threadmodel's ownership, all managered by ThreadModelManager
static std::vector<ThreadModel*> business_threadmodels;

namespace {

std::shared_ptr<ThreadModel> CreateMergeThreadModel(const DefaultThreadModelInstanceConfig& config) {
  TRPC_ASSERT(!config.instance_name.empty());

  size_t worker_thread_num = (config.io_thread_num > 0) ? config.io_thread_num : 1;
  size_t io_task_queue_size = (config.io_thread_task_queue_size > 0) ? config.io_thread_task_queue_size : 65536;

  trpc::MergeThreadModel::Options options;
  options.group_id = ThreadModelManager::GetInstance()->GenGroupId();
  options.group_name = config.instance_name;
  options.worker_thread_num = worker_thread_num;
  options.max_task_queue_size = io_task_queue_size;
  options.enable_async_io = config.enable_async_io;
  options.io_uring_entries = config.io_uring_entries;
  options.io_uring_flags = config.io_uring_flags;
  options.cpu_affinitys.clear();

  if (!config.io_cpu_affinitys.empty()) {
    TRPC_CHECK(ParseBindCoreConfig(config.io_cpu_affinitys, options.cpu_affinitys), "io_cpu_affinitys conf is error");
    options.disallow_cpu_migration = config.disallow_cpu_migration;
  }

  return std::make_shared<MergeThreadModel>(std::move(options));
}

std::shared_ptr<ThreadModel> CreateThreadModel(const DefaultThreadModelInstanceConfig& config) {
  return CreateMergeThreadModel(config);
}

void Init() {
  const GlobalConfig& global_conf = TrpcConfig::GetInstance()->GetGlobalConfig();
  const ThreadModelConfig& threadmodel_config = global_conf.threadmodel_config;
  const std::vector<DefaultThreadModelInstanceConfig>& merge_models = threadmodel_config.default_model;

  for (const auto& item : merge_models) {
    if (item.io_handle_type.compare(kMerge) != 0) {
      continue;
    }

    std::shared_ptr<ThreadModel> thread_model = CreateThreadModel(item);

    business_threadmodels.push_back(thread_model.get());

    TRPC_ASSERT(trpc::ThreadModelManager::GetInstance()->Register(thread_model));
  }
}

}  // namespace

void StartRuntime() {
  Init();

  for (auto&& it : business_threadmodels) {
    it->Start();
  }
}

void TerminateRuntime() {
  for (auto&& it : business_threadmodels) {
    it->Terminate();

    ThreadModelManager::GetInstance()->Del(it->GroupName());
  }

  business_threadmodels.clear();
}

ThreadModel* RandomGetMergeThreadModel() {
  std::size_t next = trpc::Random();
  if (business_threadmodels.size() > 0) {
    return business_threadmodels[next % business_threadmodels.size()];
  }

  return nullptr;
}

bool SubmitIoTask(ThreadModel* thread_model, MsgTask* io_task) noexcept {
  MergeThreadModel* merge_thread_model = static_cast<MergeThreadModel*>(thread_model);
  return merge_thread_model->SubmitIoTask(io_task);
}

bool SubmitHandleTask(ThreadModel* thread_model, MsgTask* handle_task) noexcept {
  MergeThreadModel* merge_thread_model = static_cast<MergeThreadModel*>(thread_model);
  return merge_thread_model->SubmitHandleTask(handle_task);
}

std::vector<Reactor*> GetReactors(ThreadModel* thread_model) noexcept {
  MergeThreadModel* merge_thread_model = static_cast<MergeThreadModel*>(thread_model);
  return merge_thread_model->GetReactors();
}

Reactor* GetReactor(ThreadModel* thread_model, int id) noexcept {
  MergeThreadModel* merge_thread_model = static_cast<MergeThreadModel*>(thread_model);
  return merge_thread_model->GetReactor(id);
}

}  // namespace trpc::merge
