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

#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "trpc/runtime/common/stats/backup_request_stats.h"
#include "trpc/runtime/common/stats/server_stats.h"

namespace trpc {

/// @brief Data statistics class of the framework.
class FrameStats {
 public:
  /// @brief Get the pointer of singleton object
  static FrameStats* GetInstance() {
    static FrameStats instance;
    return &instance;
  }

  FrameStats(const FrameStats&) = delete;
  FrameStats& operator=(const FrameStats&) = delete;

  /// @brief Start the statistical task
  void Start();

  /// @brief Stop the statistical task
  void Stop();

  /// @brief Get the statistical data of backup request
  BackupRequestStats& GetBackupRequestStats() { return backup_request_stats_; }

  /// @brief Get the statistical data of server
  ServerStats& GetServerStats() { return server_stats_; }

 private:
  FrameStats() = default;

  void Run();

 private:
  // Statistical data of server
  ServerStats server_stats_;

  // The last time when statistical data of server was printed
  uint64_t last_server_stats_time_{0};

  // the statistical data of backup request
  BackupRequestStats backup_request_stats_;

  // The last time when statistical data of backup request was reported
  uint64_t last_backup_request_time_{0};

  // task id of the statistical task
  uint64_t task_id_{0};

  // metrics plugins
  std::vector<std::string> metrics_;
};

}  // namespace trpc
