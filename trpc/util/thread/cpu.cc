//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/internal/cpu.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/thread/cpu.h"

#include <dlfcn.h>
#include <sys/sysinfo.h>
#include <syscall.h>
#include <unistd.h>

#include <atomic>
#include <climits>
#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "trpc/util/check.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/string/string_helper.h"
#include "trpc/util/thread/thread_helper.h"

namespace trpc {

namespace {

// Initialized by `InitializeProcessorInfoOnce()`.
bool inaccessible_cpus_present{false};
std::vector<int> node_of_cpus;
std::vector<std::size_t> node_index;
std::vector<unsigned> nodes_present;

bool IsValgrindPresent() {
  // You need to export this var yourself in bash.
  char* rov = getenv("RUNNING_ON_VALGRIND");
  if (rov) {
    return strcmp(rov, "0") != 0;
  }
  return false;
}

int SyscallGetCpu(unsigned* cpu, unsigned* node, void* cache) {
#ifndef SYS_getcpu
  TRPC_LOG_CRITICAL("Not supported: sys_getcpu.");
#else
  return syscall(SYS_getcpu, cpu, node, cache);
#endif
}

/// @sa: https://gist.github.com/chergert/eb6149916b10d3bf094c
int (*GetCpu)(unsigned* cpu, unsigned* node, void* cache) = [] {
  if (IsValgrindPresent()) {
    return SyscallGetCpu;
  } else {
#if defined(__aarch64__)
    return SyscallGetCpu;
#endif
    // Not all ISAs use the same name here. We'll try our best to locate
    // `getcpu` via vDSO.
    //
    // see also http://man7.org/linux/man-pages/man7/vdso.7.html for more details.

    static const char* kvDSONames[] = {"linux-gate.so.1", "linux-vdso.so.1", "linux-vdso32.so.1", "linux-vdso64.so.1"};
    static const char* kGetCpuNames[] = {"__vdso_getcpu", "__kernel_getcpu"};
    for (auto&& e : kvDSONames) {
      if (auto vdso = dlopen(e, RTLD_NOW)) {
        for (auto&& e2 : kGetCpuNames) {
          if (auto p = dlsym(vdso, e2)) {
            // AFAICT, leaking `vdso` does nothing harmful to us. We use a
            // managed pointer to hold it only to comfort Converity.
            [[maybe_unused]] static std::unique_ptr<void, int (*)(void*)> suppress_vdso_leak{vdso, &dlclose};

            return reinterpret_cast<int (*)(unsigned*, unsigned*, void*)>(p);
          }
        }
        dlclose(vdso);
      }
    }
    // Fall back to syscall. This can be slow.
    //
    // Using raw logging here as glog is unlikely to have been initialized now.
    TRPC_LOG_WARN(
        "Failed to locate `getcpu` in vDSO. Failing back to syscall. "
        "Performance will degrade.");
    return SyscallGetCpu;
  }
}();

int GetNodeOfProcessorImpl(unsigned proc_id) {
  // Slow, indeed. We don't expect this method be called much.
  std::atomic<int> rc;
  std::thread([&] {
    if (auto err = TrySetCurrentThreadAffinity({proc_id}); err != 0) {
      TRPC_CHECK(err == EINVAL, "Unexpected error #{}: {}", err, strerror(err));
      rc = -1;
      return;
    }
    unsigned cpu, node;
    TRPC_CHECK_EQ(GetCpu(&cpu, &node, nullptr), 0, "`GetCpu` failed.");
    GetCpu(&cpu, &node, nullptr);
    rc = node;
  }).join();
  return rc.load();
}

/// @brief Initialize those global variables.
void InitializeProcessorInfoOnce() {
  // I don't think it's possible to print log reliably here, unfortunately.
  static std::once_flag once;
  std::call_once(once, [&] {
    node_of_cpus.resize(GetNumberOfProcessorsConfigured(), -1);
    for (std::size_t i = 0; i != node_of_cpus.size(); ++i) {
      auto n = GetNodeOfProcessorImpl(static_cast<unsigned>(i));
      if (n == -1) {
        // Failed to determine the processor's belonging node.
        inaccessible_cpus_present = true;
      } else {
        if (node_index.size() < static_cast<std::size_t>(n + 1)) {
          node_index.resize(n + 1, numa::kUnexpectedNodeIndex);
        }
        if (node_index[n] == numa::kUnexpectedNodeIndex) {
          // New node discovered.
          node_index[n] = nodes_present.size();
          nodes_present.push_back(n);
        }
        node_of_cpus[i] = n;
        // New processor discovered.
      }
    }
  });
}

struct ProcessorInfoInitializer {
  ProcessorInfoInitializer() { InitializeProcessorInfoOnce(); }
} processor_info_initializer [[maybe_unused]];  // NOLINT

}  // namespace

namespace numa {

namespace {

std::vector<Node> GetAvailableNodesImpl() {
  // NUMA node -> vector of processor ID.
  std::unordered_map<int, std::vector<unsigned>> desc;
  for (std::size_t index = 0; index != node_of_cpus.size(); ++index) {
    auto n = node_of_cpus[index];
    if (n == -1) {
      TRPC_FMT_WARN(
          "Cannot determine node ID of processor #{}, we're silently ignoring "
          "that CPU. Unless that CPU indeed shouldn't be used by the program "
          "(e.g., containerized environment or disabled), you should check the "
          "situation as it can have a negative impact on performance.",
          index);
      continue;
    }
    desc[n].push_back(static_cast<unsigned>(index));
  }

  std::vector<Node> rc;
  for (auto&& id : nodes_present) {
    Node n;
    n.id = id;
    n.logical_cpus = desc[id];
    rc.push_back(n);
  }
  return rc;
}

}  // namespace

std::vector<Node> GetAvailableNodes() {
  static const auto rc = GetAvailableNodesImpl();
  return rc;
}

void GetCurrentProcessor(unsigned& cpu, unsigned& node) {
  unsigned cpu_id, node_id;
  // Another approach: https://stackoverflow.com/a/27450168
  TRPC_CHECK_EQ(0, GetCpu(&cpu_id, &node_id, nullptr), "Cannot get CPU and NODE ID.");
  GetCpu(&cpu_id, &node_id, nullptr);
  cpu = cpu_id;
  node = node_id;
}

unsigned GetCurrentNode() {
  unsigned cpu, node;
  GetCurrentProcessor(cpu, node);
  return node;
}

std::size_t GetCurrentNodeIndex() {
  unsigned cpu, node;
  GetCurrentProcessor(cpu, node);
  return GetNodeIndexOf(node, &cpu);
}

int GetNodeId(std::size_t index) {
  TRPC_CHECK_LT(index, nodes_present.size());
  return nodes_present[index];
}

std::size_t GetNodeIndexOf(unsigned node_id, unsigned* cpu) {
  // FIXME. It seems that there are some bugs on k8s POD: sometimes an UNASSIGNED
  // numa core will be dispatched for running thread. This is a compatible solution.
  auto counter_f = [](unsigned node_id, unsigned* cpu) {
    // counter is NOT a map for every unexpected node_id
    static std::atomic<std::size_t> counter{0};
    counter.fetch_add(1, std::memory_order_relaxed);
    int cpu_id = cpu ? *cpu : -1;
    TRPC_FMT_WARN_IF(TRPC_EVERY_N(1000), "Get unexpected NUMA ID #{} (CPU #{}), totally {} times.", node_id, cpu_id,
                     counter.load(std::memory_order_relaxed));
  };

  if (TRPC_UNLIKELY(static_cast<std::size_t>(node_id) >= node_index.size())) {
    counter_f(node_id, cpu);
    return kUnexpectedNodeIndex;
  }

  auto index = node_index[node_id];
  if (TRPC_UNLIKELY(index == kUnexpectedNodeIndex)) {
    counter_f(node_id, cpu);
  } else {
    TRPC_CHECK(index < nodes_present.size());
  }

  return index;
}

std::size_t GetNumberOfNodesAvailable() { return nodes_present.size(); }

int GetNodeOfProcessor(unsigned cpu) {
  TRPC_CHECK_LE(static_cast<std::size_t>(cpu), node_of_cpus.size());
  auto node = node_of_cpus[cpu];
  TRPC_CHECK_NE(node, -1, "Processor #{} is not accessible.", cpu);
  return node;
}

}  // namespace numa

unsigned GetCurrentProcessorId() {
  unsigned cpu, node;
  numa::GetCurrentProcessor(cpu, node);
  return cpu;
}

std::size_t GetNumberOfProcessorsAvailable() {
  // We do not support CPU hot-plugin, so we use `static` here.
  static const auto rc = get_nprocs();
  return rc;
}

std::size_t GetNumberOfProcessorsConfigured() {
  // We do not support CPU hot-plugin, so we use `static` here.
  static const auto rc = get_nprocs_conf();
  return rc;
}

bool IsInaccessibleProcessorPresent() { return inaccessible_cpus_present; }

bool IsProcessorAccessible(unsigned cpu) { return node_of_cpus[cpu] != -1; }

std::optional<std::vector<unsigned>> TryParseProcesserList(const std::string& s) {
  std::vector<unsigned> result;
  auto parts = Split(s, ",");
  for (auto&& e : parts) {
    auto id = TryParse<int>(e);
    if (id) {
      if (*id < 0) {
        int acutal = GetNumberOfProcessorsConfigured() + *id;
        if (acutal < 0) {
          return std::nullopt;
        }
        result.push_back(acutal);
      } else {
        result.push_back(*id);
      }
    } else {
      auto range = Split(e, "-");
      if (range.size() != 2) {
        return std::nullopt;
      }
      auto s = TryParse<int>(range[0]), e = TryParse<int>(range[1]);
      if (!s || !e || *s > *e) {
        return std::nullopt;
      }
      for (int i = *s; i <= *e; ++i) {
        result.push_back(i);
      }
    }
  }
  return result;
}

}  // namespace trpc
