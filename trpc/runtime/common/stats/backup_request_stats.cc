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

#include "trpc/runtime/common/stats/backup_request_stats.h"

#include "trpc/metrics/metrics_factory.h"
#include "trpc/util/log/logging.h"

namespace trpc {

BackupRequestStats::Data BackupRequestStats::GetData(const std::string& service_name) {
  std::unique_lock<std::mutex> lc(service_mutex_);
  if (auto itr = service_data_.find(service_name); itr != service_data_.end()) {
    return itr->second;
  }

  BackupRequestStats::Data data;
  std::string path{"trpc/client/"};
  path.append(service_name);

  std::string retries_path = path + "/backup_request";
  data.retries = std::make_shared<tvar::Counter<uint64_t>>(retries_path);

  std::string retries_success_path = path + "/backup_request_success";
  data.retries_success = std::make_shared<tvar::Counter<uint64_t>>(retries_success_path);

  service_data_[service_name] = data;

  return data;
}

void BackupRequestStats::CheckReportData(const std::shared_ptr<tvar::Counter<uint64_t>>& retries,
                                         const std::shared_ptr<tvar::Counter<uint64_t>>& retries_success,
                                         std::unordered_map<std::string, uint64_t>& report_values) {
  auto abs_path = retries->GetAbsPath();
  auto value = retries->GetValue();
  // If the backup request is not triggered, do not report.
  if (value == 0) {
    return;
  }
  report_values.emplace(std::move(abs_path), value);
  report_values.emplace(retries_success->GetAbsPath(), retries_success->GetValue());
}

void BackupRequestStats::AsyncReportToMetrics(const std::string& metrics_name) {
  if (metrics_name.empty()) {
    return;
  }

  auto metrics_ptr = MetricsFactory::GetInstance()->Get(metrics_name);
  if (!metrics_ptr) {
    return;
  }

  std::unordered_map<std::string, uint64_t> report_values;
  report_values.reserve(service_data_.size() * 2);
  {
    std::unique_lock<std::mutex> lc(service_mutex_);
    for (auto& kv : service_data_) {
      CheckReportData(kv.second.retries, kv.second.retries_success, report_values);
    }
  }

  if (report_values.empty()) {
    return;
  }

  for (auto& kv : report_values) {
    auto now_value = kv.second;
    auto last_value = last_backup_request_value_[kv.first];
    last_backup_request_value_[kv.first] = now_value;

    auto diff_value = now_value >= last_value ? (now_value - last_value) : now_value;

    SingleAttrMetricsInfo single_attr_info;
    single_attr_info.dimension = kv.first;
    single_attr_info.value = static_cast<double>(diff_value);
    metrics_ptr->SingleAttrReport(single_attr_info);
    TRPC_FMT_TRACE("BackupRequestTvar report dimension:{} value:{}", single_attr_info.dimension,
                   single_attr_info.value);
  }
}

}  // namespace trpc
