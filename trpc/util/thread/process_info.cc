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

#include "trpc/util/thread/process_info.h"

#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "trpc/util/string/string_util.h"

namespace trpc {

namespace detail {

/// @brief The path to the dockerenv file.
constexpr char kDockerEnvPath[] = "/.dockerenv";

/// @brief The file path of the process Cgroup. For file description, please refer to cgroup(7).
constexpr char kProcSelfCgroupPath[] = "/proc/self/cgroup";

/// @brief The file path of the process mount information. For file description, please refer to proc(5).
constexpr char kProcSelfMountInfoPath[] = "/proc/self/mountinfo";

/// @brief The CPU bandwidth that can be used by the Cgroup, measured in microseconds.
constexpr char kCgroupCPUCFSQuotaUsParam[] = "cpu.cfs_quota_us";

/// @brief One CPU bandwidth, measured in microseconds.
constexpr char kCgroupCPUCFSPeriodUsParam[] = "cpu.cfs_period_us";

/// @brief The memory limit that can be used by the Cgroup, measured in bytes.
constexpr char kCgroupMemoryLimitBytesParam[] = "memory.limit_in_bytes";

/// @brief Check if the /.dockerenv file exists.
/// @return bool type true: exists; false: does not exist
bool HasDockerEnvPath() {
  // Docker will create a .dockerenv file in the root directory inside the container,
  // but customized Docker systems may not have this file.
  struct stat file_stat;
  return (stat(kDockerEnvPath, &file_stat) == 0);
}

/// @brief Check if the container identification is included in the process Cgroup.
/// @return bool type true: exists; false: does not exist
bool HasContainerCgroups() {
  std::ifstream file_stream(kProcSelfCgroupPath);
  if (!file_stream.is_open()) {
    return false;
  }
  std::stringstream buffer;
  buffer << file_stream.rdbuf();
  std::string file_contents(buffer.str());

  // The Cgroup hierarchy path of the container starts with `docker/kubepods`.
  bool has_docker_flag = file_contents.find(":/docker/") != std::string::npos;
  bool has_kubepods_flag = file_contents.find(":/kubepods/") != std::string::npos;
  return has_docker_flag || has_kubepods_flag;
}

/// @brief Subsystem information of a process.
struct CGroupSubsystem {
  // Subsystem name.
  std::string name;
  // Cgroup hierarchy ID.
  int hierarchy_id;
  // Path name in the Cgroup hierarchy.
  std::string path_name;
};

/// @brief Subsystem mount information.
struct SubsystemMountPoint {
  // Subsystem name.
  std::string name;
  // Root path of the Cgroup hierarchy.
  std::string root;
  // Mount point.
  std::string mount_point;
};

/// @brief Get all subsystems of a process.
/// @return The type of std::optional,which is std::nullopt, it means failure; otherwise, it means success.
std::optional<std::unordered_map<std::string, CGroupSubsystem>> GetProcessCgroupSubSystems() {
  std::ifstream file_stream(kProcSelfCgroupPath);
  if (!file_stream.is_open()) {
    return std::nullopt;
  }

  std::unordered_map<std::string, CGroupSubsystem> subsystems;
  std::string line;
  // The content of each line is: hierarchy-ID:controller-list:cgroup-path.
  while (getline(file_stream, line)) {
    std::vector<std::string> infos = util::SplitString(line, ':');
    if (infos.size() != 3) {
      continue;
    }
    std::vector<std::string> control_list = util::SplitString(infos[1], ',');
    for (auto& subsystem_name : control_list) {
      subsystems[subsystem_name] =
          CGroupSubsystem{.name = subsystem_name, .hierarchy_id = std::atoi(infos[0].c_str()), .path_name = infos[2]};
    }
  }
  return subsystems;
}

/// @brief Get the subsystem information of the process corresponding to `subsystem_name`.
/// @return The type of std::optional,which is std::nullopt, it means failure; otherwise, it means success.
std::optional<CGroupSubsystem> GetProcessSubsystem(const std::string& subsystem_name) {
  auto subsystems = GetProcessCgroupSubSystems();
  if (!subsystems || ((*subsystems).find(subsystem_name) == (*subsystems).end())) {
    return std::nullopt;
  }
  return (*subsystems)[subsystem_name];
}

/// @brief Parse the required mount information based on the content read from `/proc/self/cgroup`.
/// @return The type of std::optional,which is std::nullopt, it means failure; otherwise, it means success.
std::optional<SubsystemMountPoint> ParseMountPoint(const std::string& line, const std::string& target_name) {
  if (line.find(target_name) == std::string::npos) {
    return std::nullopt;
  }
  // Each line contains 11 fields. The third field is the root path of the cgroup hierarchy; the fourth field is
  // the mount path corresponding to the root; the tenth field is the super parameter, which contains the subsystem
  // name.
  std::vector<std::string> infos = util::SplitString(line, ' ');
  if (infos.size() != 11) {
    return std::nullopt;
  }

  std::optional<SubsystemMountPoint> res = std::nullopt;
  std::vector<std::string> super_options = util::SplitString(infos[10], ',');
  for (auto& option : super_options) {
    if (option == target_name) {  // Find the corresponding subsystem.
      SubsystemMountPoint mount_point;
      mount_point.name = target_name;
      mount_point.root = infos[3];
      mount_point.mount_point = infos[4];
      res = mount_point;
      break;
    }
  }
  return res;
}

/// @brief Get the mount information of the subsystem corresponding to `subsystem_name`.
/// @return The type of std::optional,which is std::nullopt, it means failure; otherwise, it means success.
std::optional<SubsystemMountPoint> GetSubsystemMountPoint(const std::string& subsystem_name) {
  std::ifstream file_stream(kProcSelfMountInfoPath);
  if (!file_stream.is_open()) {
    return std::nullopt;
  }

  std::optional<SubsystemMountPoint> res = std::nullopt;
  std::string line;
  while (getline(file_stream, line)) {
    auto mount_point = ParseMountPoint(line, subsystem_name);
    if (mount_point.has_value()) {
      res = mount_point;
      break;
    }
  }
  return res;
}

/// @brief Check the correctness of the cgroup hierarchy path.
/// @return bool type true: success; false: failure
bool CheckSubsystemPathValid(std::string_view base_path, std::string_view target_path) {
  if (target_path.size() < base_path.size()) {
    return false;
  }
  if (target_path.substr(0, base_path.size()) != base_path) {
    return false;
  }
  return true;
}

/// @brief Get the hierarchical mount path of the `subsystem_name` subsystem for the process.
/// @return The type of std::optional,which is std::nullopt, it means failure; otherwise, it means success.
std::optional<std::string> GetSubsystemMountPath(const std::string& subsystem_name) {
  // Get the subsystems of the process.
  auto subsystem = GetProcessSubsystem(subsystem_name);
  // Get the mount information of the subsystem.
  auto mount_point = GetSubsystemMountPoint(subsystem_name);
  if (!subsystem || !mount_point) {
    return std::nullopt;
  }

  // Check the correctness of the hierarchy path, where `subsystem.path_name` should be equal to `mount_point.root`
  // or its subdirectory.
  if (!CheckSubsystemPathValid((*mount_point).root, (*subsystem).path_name)) {
    return std::nullopt;
  }

  // Construct the hierarchical mount path of the `subsystem_name subsystem` for the process,
  // which is the root mount point plus the relative path of the hierarchy.
  std::string mount_path = (*mount_point).mount_point + "/" + (*subsystem).path_name.substr((*mount_point).root.size());
  return mount_path;
}

/// @brief Get the value of the specified parameter in cgroup.
/// @param mount_path Mount path
/// @param param_name Specified parameter name
/// @return The type of std::optional,which is std::nullopt, it means failure; otherwise, it means success.
std::optional<int64_t> GetCgroupInt64Param(const std::string& mount_path, const std::string& param_name) {
  std::ifstream file_stream(mount_path + "/" + param_name);
  if (!file_stream.is_open()) {
    return std::nullopt;
  }

  std::string line;
  if (getline(file_stream, line)) {
    return std::atoll(line.c_str());
  }
  return std::nullopt;
}

/// @brief Get the CPU quota of the container.
/// @return The type of std::optional,which is std::nullopt, it means failure; otherwise, it means success.
std::optional<double> GetContainerCpuQuota() {
  auto mount_path = GetSubsystemMountPath("cpu");
  if (!mount_path) {
    return std::nullopt;
  }

  // Get CPU bandwidth information.
  auto cpu_quota_us = GetCgroupInt64Param(*mount_path, kCgroupCPUCFSQuotaUsParam);
  auto cpu_period_us = GetCgroupInt64Param(*mount_path, kCgroupCPUCFSPeriodUsParam);
  if (!cpu_quota_us || !cpu_period_us) {
    return std::nullopt;
  }

  // Calculate the CPU quota, where `cpu_quota_us` equals -1 indicates no limit,
  // and in other cases it is equal to `cpu.cfs_quota_us / cpu.cfs_period_us`.
  if (*cpu_quota_us == -1) {  // Unlimited
    return static_cast<double>(sysconf(_SC_NPROCESSORS_ONLN));
  }
  if (*cpu_period_us != 0) {
    return static_cast<double>(*cpu_quota_us) / (*cpu_period_us);
  }
  return std::nullopt;
}

/// @brief Get the memory quota of the container.
/// @return The type of std::optional,which is std::nullopt, it means failure; otherwise, it means success.
std::optional<int64_t> GetContainerMemoryQuota() {
  auto mount_path = GetSubsystemMountPath("memory");
  if (!mount_path) {
    return std::nullopt;
  }

  // Get the memory quota
  auto memory_quota = GetCgroupInt64Param(*mount_path, kCgroupMemoryLimitBytesParam);
  if (!memory_quota) {
    return std::nullopt;
  }
  return *memory_quota;
}
}  // namespace detail

bool IsProcessInContainer() { return detail::HasDockerEnvPath() || detail::HasContainerCgroups(); }

double GetProcessCpuQuota(const std::function<bool()>& check_container) {
  if (!check_container || !check_container()) {
    // Return the CPU configuration of the system in a non-container environment.
    return static_cast<double>(sysconf(_SC_NPROCESSORS_ONLN));
  }
  auto cpu_quota = detail::GetContainerCpuQuota();
  if (!cpu_quota) {
    // Failed to retrieve Cgroup quota, returning the CPU configuration of the system.
    return static_cast<double>(sysconf(_SC_NPROCESSORS_ONLN));
  }
  return *cpu_quota;
}

int64_t GetProcessMemoryQuota(const std::function<bool()>& check_container) {
  if (!check_container || !check_container()) {
    // Return the CPU configuration of the system in a non-container environment.
    return static_cast<int64_t>(sysconf(_SC_PAGESIZE)) * static_cast<int64_t>(sysconf(_SC_PHYS_PAGES));
  }
  auto memory_quota = detail::GetContainerMemoryQuota();
  if (!memory_quota) {
    // Failed to retrieve Cgroup quota, returning the CPU configuration of the system.
    return static_cast<int64_t>(sysconf(_SC_PAGESIZE)) * static_cast<int64_t>(sysconf(_SC_PHYS_PAGES));
  }
  return *memory_quota;
}

}  // namespace trpc
