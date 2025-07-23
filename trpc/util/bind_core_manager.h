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
#include <string>
#include <vector>

namespace trpc {

/// @brief Provide core binding function to bind threads to specific logical CPUs. 
///        It can be used for global core binding or user thread core binding.
/// @note  This class is to be compatible with previous versions of usage.
///        You can use the interfaces in 'thread_helper.h' to achieve the same effect.
class BindCoreManager {
 public:
  /// @brief Parse the core binding configuration and check its validity.
  /// @param [in] bind_core_conf configuration string
  /// @param [out] bind_core_group store the set of logical CPU numbers that the thread needs to be bound to
  /// @return Return true on success, otherwise return false.
  static bool ParseBindCoreConf(const std::string& bind_core_conf, std::vector<uint32_t>& bind_core_group);

  /// @brief Parse and check the global core binding configuration of the framework.
  /// @param bind_core_conf Core binding parameter configured in the framework configuration.
  /// @return Return true on success, otherwise return false.
  /// @note This interface is only used for parsing and checking the global core binding configuration of the framework.
  ///       The parsed result will be stored in bind_core_group_.
  static bool ParseBindCoreGroup(const std::string& bind_core_conf);

  /// @brief Bind to the logical CPUs according to the core binding configuration. If the configuration is empty, bind
  ///        to the available CPUs on the machine.
  ///        The implementation adopts a strategy of binding cores in a loop from start to finish.
  /// @return On success, return the CPU numbers that have been successfully bound. Otherwise return -1.
  /// @note This interface is only used for parsing and checking the global core binding configuration of the framework.
  static int BindCore();

  /// @brief Get the CPU affinity set in the configuration file.
  static const std::vector<unsigned>& GetAffinity() { return bind_core_group_; }

  /// @brief Provide the ability to customize core binding for users,  same as ParseBindCoreGroup
  static bool UserParseBindCoreGroup(const std::string& bind_core_conf);

  /// @brief Provide the ability to customize core binding for users,  same as BindCore
  static int UserBindCore();

 private:
  // used to implement atomic variables for cyclically binding CPU cores
  static std::atomic<uint64_t> s_cpu_seq_, s_cpu_seq_user_;

  // store the set of logical CPU numbers that the thread needs to be bound to
  static std::vector<uint32_t> bind_core_group_, bind_core_group_user_;
};

}  // namespace trpc
