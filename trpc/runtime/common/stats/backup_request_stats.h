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
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "trpc/tvar/basic_ops/reducer.h"

namespace trpc {

/// @brief Data statistics class of backup-request, in service level.
class BackupRequestStats {
 public:
  /// Report interval
  static constexpr uint64_t kBackupRequestIntervalMs{3000};

  /// @brief Statistical data of backup request
  struct Data {
    /// retry count of backup-request
    std::shared_ptr<tvar::Counter<uint64_t>> retries;
    /// success count caused by retry request of backup-request
    std::shared_ptr<tvar::Counter<uint64_t>> retries_success;
  };

  /// @brief Get statistical data of backup-request
  Data GetData(const std::string& service_name);

  /// @brief Async report statistical data of backup-request
  /// @param metrics_name metrics plugin name
  void AsyncReportToMetrics(const std::string& metrics_name);

 private:
  // Check and get report data
  void CheckReportData(const std::shared_ptr<tvar::Counter<uint64_t>>& retries,
                       const std::shared_ptr<tvar::Counter<uint64_t>>& retries_success,
                       std::unordered_map<std::string, uint64_t>& report_values);

 private:
  // key is service name, value is statistical data
  std::unordered_map<std::string, Data> service_data_;

  std::mutex service_mutex_;

  // last statistical data of backup-request
  std::unordered_map<std::string, uint64_t> last_backup_request_value_;
};

}  // namespace trpc
