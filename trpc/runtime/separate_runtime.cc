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

#include "trpc/runtime/separate_runtime.h"

#include "trpc/common/config/global_conf.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/runtime/threadmodel/separate/non_steal/non_steal_scheduling.h"
#include "trpc/runtime/threadmodel/separate/separate_thread_model.h"
#include "trpc/runtime/threadmodel/separate/steal/steal_scheduling.h"
#include "trpc/runtime/threadmodel/thread_model_manager.h"
#include "trpc/util/check.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/random.h"
#include "trpc/util/thread/thread_helper.h"

namespace trpc::separate {

// admin_threadmodel and business_threadmodels has not owned threadmodel's ownership
// all managered by ThreadModelManager
static ThreadModel* admin_threadmodel{nullptr};
static std::vector<ThreadModel*> business_threadmodels;

namespace {

NonStealScheduling::Options CreateNonStealSchedulingOptions(const DefaultThreadModelInstanceConfig& config) {
  TRPC_ASSERT(!config.instance_name.empty());

  size_t handle_thread_num = (config.handle_thread_num > 0) ? config.handle_thread_num : 2;

  NonStealScheduling::Options options;
  options.group_name = config.instance_name;
  options.worker_thread_num = handle_thread_num;
  options.local_queue_size = config.scheduling.local_queue_size;
  options.global_queue_size = config.handle_thread_task_queue_size;

  return options;
}

StealScheduling::Options CreateStealSchedulingOptions(const DefaultThreadModelInstanceConfig& config) {
  TRPC_ASSERT(!config.instance_name.empty());

  size_t handle_thread_num = (config.handle_thread_num > 0) ? config.handle_thread_num : 2;

  StealScheduling::Options options;
  options.group_name = config.instance_name;
  options.worker_thread_num = handle_thread_num;
  options.global_queue_size = config.handle_thread_task_queue_size;

  return options;
}

bool RegisterScheduling(const std::string& scheduling_name, const DefaultThreadModelInstanceConfig& config) {
  if (scheduling_name == std::string(kNonStealScheduling)) {
    SeparateSchedulingCreateFunction scheduling_create_func = [&config]() mutable {
      NonStealScheduling::Options options = CreateNonStealSchedulingOptions(config);
      std::unique_ptr<SeparateScheduling> separate_scheduling =
        std::make_unique<separate::NonStealScheduling>(std::move(options));
      return separate_scheduling;
    };

    return RegisterSeparateSchedulingImpl(scheduling_name, std::move(scheduling_create_func));
  } else if (scheduling_name == std::string(kStealScheduling)) {
    SeparateSchedulingCreateFunction scheduling_create_func = [&config]() mutable {
      StealScheduling::Options options = CreateStealSchedulingOptions(config);
      std::unique_ptr<SeparateScheduling> separate_scheduling =
        std::make_unique<separate::StealScheduling>(std::move(options));
      return separate_scheduling;
    };

    return RegisterSeparateSchedulingImpl(scheduling_name, std::move(scheduling_create_func));
  }

  return false;
}

std::shared_ptr<ThreadModel> CreateSeparateThreadModel(const DefaultThreadModelInstanceConfig& config) {
  TRPC_ASSERT(!config.instance_name.empty());

  size_t io_thread_num = (config.io_thread_num > 0) ? config.io_thread_num : 2;
  size_t handle_thread_num = (config.handle_thread_num > 0) ? config.handle_thread_num : 2;
  std::string scheduling_name = (!config.scheduling.scheduling_name.empty())
                                ? config.scheduling.scheduling_name
                                : std::string(kNonStealScheduling);

  TRPC_ASSERT(RegisterScheduling(scheduling_name, config));

  size_t io_task_queue_size = (config.io_thread_task_queue_size > 0) ? config.io_thread_task_queue_size : 65536;

  trpc::SeparateThreadModel::Options options;
  options.group_id = ThreadModelManager::GetInstance()->GenGroupId();
  options.group_name = config.instance_name;
  options.scheduling_name = scheduling_name;
  options.io_thread_num = io_thread_num;
  options.handle_thread_num = handle_thread_num;
  options.io_task_queue_size = io_task_queue_size;
  options.enable_async_io = config.enable_async_io;
  options.io_uring_entries = config.io_uring_entries;
  options.io_uring_flags = config.io_uring_flags;
  options.handle_cpu_affinitys.clear();
  options.io_cpu_affinitys.clear();

  bool is_check = (config.handle_cpu_affinitys.empty() && config.io_cpu_affinitys.empty());
  is_check |= (!config.handle_cpu_affinitys.empty() && !config.io_cpu_affinitys.empty());

  TRPC_CHECK(is_check, "Don`t Set Only One of `handle_cpu_affinitys` and `io_cpu_affinitys`");

  if (!config.handle_cpu_affinitys.empty()) {
    TRPC_CHECK(ParseBindCoreConfig(config.handle_cpu_affinitys, options.handle_cpu_affinitys),
               "handle_cpu_affinitys conf is error");
  }

  if (!config.io_cpu_affinitys.empty()) {
    TRPC_CHECK(ParseBindCoreConfig(config.io_cpu_affinitys, options.io_cpu_affinitys),
               "io_cpu_affinitys conf is error");
  }

  if (!options.handle_cpu_affinitys.empty() && !options.io_cpu_affinitys.empty()) {
    options.disallow_cpu_migration = config.disallow_cpu_migration;
  }


  return std::make_shared<SeparateThreadModel>(std::move(options));
}

std::shared_ptr<ThreadModel> CreateThreadModel(const DefaultThreadModelInstanceConfig& config) {
  return CreateSeparateThreadModel(config);
}

void InitBusinessThreadModel() {
  const GlobalConfig& global_conf = TrpcConfig::GetInstance()->GetGlobalConfig();
  const ThreadModelConfig& threadmodel_config = global_conf.threadmodel_config;
  const std::vector<DefaultThreadModelInstanceConfig>& separate_models = threadmodel_config.default_model;

  for (const auto& item : separate_models) {
    if (item.io_handle_type.compare(kSeparate) != 0) {
      continue;
    }

    // cannot naming same as kSeparateAdminInstance
    TRPC_ASSERT(item.instance_name.compare(kSeparateAdminInstance) != 0);

    std::shared_ptr<ThreadModel> thread_model = CreateThreadModel(item);

    business_threadmodels.push_back(thread_model.get());

    TRPC_ASSERT(ThreadModelManager::GetInstance()->Register(thread_model));
  }
}

void InitAdminThreadModel() {
  DefaultThreadModelInstanceConfig admin_threadmodel_config;
  admin_threadmodel_config.instance_name = kSeparateAdminInstance;
  admin_threadmodel_config.io_thread_num = 1;
  admin_threadmodel_config.handle_thread_num = 1;

  std::shared_ptr<ThreadModel> thread_model = CreateThreadModel(admin_threadmodel_config);
  admin_threadmodel = thread_model.get();

  TRPC_ASSERT(ThreadModelManager::GetInstance()->Register(thread_model));
}

}  // namespace

void StartAdminRuntime() {
  InitAdminThreadModel();

  admin_threadmodel->Start();
}

void TerminateAdminRuntime() {
  admin_threadmodel->Terminate();

  ThreadModelManager::GetInstance()->Del(admin_threadmodel->GroupName());

  admin_threadmodel = nullptr;
}

void StartRuntime() {
  InitBusinessThreadModel();

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

ThreadModel* RandomGetSeparateThreadModel() {
  if (business_threadmodels.size()) {
    std::size_t next = trpc::Random();
    return business_threadmodels[next % business_threadmodels.size()];
  }

  return admin_threadmodel;
}

bool SubmitIoTask(ThreadModel* thread_model, MsgTask* io_task) noexcept {
  TRPC_ASSERT(thread_model);
  SeparateThreadModel* separate_thread_model = static_cast<SeparateThreadModel*>(thread_model);
  return separate_thread_model->SubmitIoTask(io_task);
}

bool SubmitHandleTask(ThreadModel* thread_model, MsgTask* handle_task) noexcept {
  TRPC_ASSERT(thread_model);
  SeparateThreadModel* separate_thread_model = static_cast<SeparateThreadModel*>(thread_model);
  return separate_thread_model->SubmitHandleTask(handle_task);
}

std::vector<Reactor*> GetReactors(ThreadModel* thread_model) noexcept {
  TRPC_ASSERT(thread_model);
  SeparateThreadModel* separate_thread_model = static_cast<SeparateThreadModel*>(thread_model);
  return separate_thread_model->GetReactors();
}

Reactor* GetReactor(ThreadModel* thread_model, int id) noexcept {
  TRPC_ASSERT(thread_model);
  SeparateThreadModel* separate_thread_model = static_cast<SeparateThreadModel*>(thread_model);
  return separate_thread_model->GetReactor(id);
}

}  // namespace trpc::separate
