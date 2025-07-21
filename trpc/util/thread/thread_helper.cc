//
//
// Copyright (C) 2020 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/thread/attribute.cc.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/thread/thread_helper.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pthread.h>
#include <sched.h>
#include <sys/prctl.h>

#include <algorithm>
#include <set>

#include "trpc/util/check.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/string_helper.h"
#include "trpc/util/string_util.h"

namespace trpc {

int TrySetCurrentThreadAffinity(const std::vector<unsigned>& affinity) {
  TRPC_CHECK(!affinity.empty());
  cpu_set_t cpuset;

  CPU_ZERO(&cpuset);
  for (auto&& e : affinity) {
    CPU_SET(e, &cpuset);
  }
  return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

void SetCurrentThreadAffinity(const std::vector<unsigned>& affinity) {
  int rc = TrySetCurrentThreadAffinity(affinity);
  TRPC_CHECK(rc == 0, "Cannot set thread affinity for thread [{}]: [{}] {}.",
             reinterpret_cast<const void*>(pthread_self()), rc, strerror(rc));
}

int TryGetCurrentThreadAffinity(std::vector<unsigned>& affinity) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  int rc = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (rc) {
    return -1;
  }

  for (unsigned i = 0; i != __CPU_SETSIZE; ++i) {
    if (CPU_ISSET(i, &cpuset)) {
      affinity.push_back(i);
    }
  }
  return rc;
}

std::vector<unsigned> GetCurrentThreadAffinity() {
  std::vector<unsigned> result;
  int rc = TryGetCurrentThreadAffinity(result);
  TRPC_CHECK(rc == 0, "Cannot get thread affinity of thread [{}]: [{}] {}.",
             reinterpret_cast<const void*>(pthread_self()), rc, strerror(rc));
  return result;
}

void SetCurrentThreadName(const std::string& name) {
  int rc = pthread_setname_np(pthread_self(), name.c_str());
  if (rc == ERANGE) {
    // If the thread name length exceeds the limit, use prctl. 
    // To support scenarios where thread names exceed 16 bytes in size. 
    char thread_name[64] = {0};
    snprintf(thread_name, sizeof(thread_name), "%s", name.c_str());
    rc = prctl(PR_SET_NAME, thread_name, NULL, NULL, NULL);
  }
  TRPC_CHECK(rc == 0, "Cannot set name [{}] for thread [{}]: [{}] {}", name,
             reinterpret_cast<const void*>(pthread_self()), rc, strerror(rc));
}

namespace {

// Check if the bound CPU is valid.
bool CheckBindCoreGroupEffective(std::vector<uint32_t>& bind_core_group) {
  // get the cpu numbers of system
  std::vector<uint32_t> cpu_eff_ids;
  if (TryGetCurrentThreadAffinity(cpu_eff_ids) || bind_core_group.empty()) {
    return false;
  }
  // sort the CPU numbers in ascending order.
  std::sort(bind_core_group.begin(), bind_core_group.end());
  std::sort(cpu_eff_ids.begin(), cpu_eff_ids.end());

  return std::includes(cpu_eff_ids.begin(), cpu_eff_ids.end(), bind_core_group.begin(), bind_core_group.end());
}

}  // namespace

// Check if the parameters in bind_core_conf are valid and populate the bind_core_group
// An input example of '1,5-7' will be converted to a list of [1, 5, 6, 7].
bool ParseBindCoreConfig(const std::string& bind_core_conf, std::vector<uint32_t>& bind_core_group) {
  std::vector<uint32_t> core_group;

  auto parts = Split(bind_core_conf, ",");
  for (auto&& e : parts) {
    auto range = Split(e, "-");
    if (range.size() == 1) {
      auto id = TryParse<uint32_t>(range[0]);
      TRPC_CHECK(id, "Unrecognized CPU ID [{}].", range[0]);
      core_group.push_back(*id);
    } else {
      TRPC_CHECK_EQ(std::size_t(2), range.size(), "Unrecognized CPU range [{}].", e);
      auto start = TryParse<uint32_t>(range[0]), end = TryParse<uint32_t>(range[1]);
      TRPC_CHECK(start && end, "Unrecognized CPU range [{}].", *end);
      TRPC_CHECK(*start <= *end, "CPU range must be sorted from smallest to largest {}", e);
      for (uint32_t i = *start; i <= *end; ++i) {
        core_group.push_back(i);
      }
    }
  }

  // If there are duplicate elements, return failure directly.
  if (std::set<uint32_t>(core_group.begin(), core_group.end()).size() != core_group.size()) {
    TRPC_FMT_ERROR("bind_core_group:{} include the repeat CPU id", bind_core_conf);
    return false;
  }

  // Check if all CPUs in bind_core_group_ are valid.
  if (CheckBindCoreGroupEffective(core_group)) {
    bind_core_group.insert(bind_core_group.end(), core_group.begin(), core_group.end());
    return true;
  }

  TRPC_FMT_ERROR("bind_core_group:{} include ineffective CPU id", bind_core_conf);

  return false;
}

}  // namespace trpc
