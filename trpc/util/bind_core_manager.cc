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

#include "trpc/util/bind_core_manager.h"

#include "trpc/util/log/logging.h"
#include "trpc/util/thread/thread_helper.h"

namespace trpc {

std::atomic<uint64_t> BindCoreManager::s_cpu_seq_(0);
std::atomic<uint64_t> BindCoreManager::s_cpu_seq_user_(0);
std::vector<uint32_t> BindCoreManager::bind_core_group_;
std::vector<uint32_t> BindCoreManager::bind_core_group_user_;

bool BindCoreManager::ParseBindCoreConf(const std::string& bind_core_conf, std::vector<uint32_t>& bind_core_group) {
  return ParseBindCoreConfig(bind_core_conf, bind_core_group);
}

bool BindCoreManager::ParseBindCoreGroup(const std::string& bind_core_conf) {
  bool ret = false;
  bind_core_group_.clear();
  if (!bind_core_conf.empty()) {
    ret = ParseBindCoreConfig(bind_core_conf, bind_core_group_);
    if (!ret) {
      TRPC_FMT_ERROR("Invalid config[global->bind_core_group]:{}, close bind core policy", bind_core_conf);
      bind_core_group_.clear();
    } else {
      s_cpu_seq_.fetch_and(0);
    }
  }
  return ret;
}

bool BindCoreManager::UserParseBindCoreGroup(const std::string& bind_core_conf) {
  bool ret = false;
  bind_core_group_user_.clear();
  if (!bind_core_conf.empty()) {
    ret = ParseBindCoreConfig(bind_core_conf, bind_core_group_user_);
    if (!ret) {
      TRPC_FMT_ERROR("Invalid config:{}, close bind core policy", bind_core_conf);
      bind_core_group_user_.clear();
    } else {
      s_cpu_seq_user_.fetch_and(0);
    }
  }
  return ret;
}

int BindCoreImpl(const std::vector<uint32_t>& bind_core_group, std::atomic<uint64_t>* cpu_seq) {
  std::vector<uint32_t> cpu_eff_ids{};

  if (bind_core_group.empty()) {
    TryGetCurrentThreadAffinity(cpu_eff_ids);
  } else {
    cpu_eff_ids = bind_core_group;
  }

  if (cpu_eff_ids.empty()) {
    return -1;
  }

  int cpu = cpu_seq->fetch_add(1);
  cpu = cpu % cpu_eff_ids.size();

  if (TrySetCurrentThreadAffinity({cpu_eff_ids[cpu]})) {
    return -1;
  }

  return cpu_eff_ids[cpu];
}

int BindCoreManager::BindCore() { return BindCoreImpl(bind_core_group_, &s_cpu_seq_); }

int BindCoreManager::UserBindCore() { return BindCoreImpl(bind_core_group_user_, &s_cpu_seq_user_); }

}  // namespace trpc
