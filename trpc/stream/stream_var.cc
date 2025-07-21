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

#include "trpc/stream/stream_var.h"

#include <utility>

#include "trpc/metrics/metrics_factory.h"
#include "trpc/util/log/logging.h"

namespace trpc::stream {

namespace {

constexpr std::string_view kStreamRuntimeInfo = "trpc_stream_runtime_info";

} // namespace

StreamVarPtr CreateStreamVar(const std::string& var_path) { return MakeRefCounted<StreamVar>(var_path); }

StreamVar::StreamVar(const std::string& var_path)
    : rpc_call_count_(var_path + "/rpc_call_count"),
      rpc_call_failure_count_(var_path + "/rpc_call_failure_count"),
      send_msg_count_(var_path + "/send_msg_count"),
      recv_msg_count_(var_path + "/recv_msg_count"),
      send_msg_bytes_(var_path + "/send_msg_bytes"),
      recv_msg_bytes_(var_path + "/recv_msg_bytes") {
  CaptureStreamVarSnapshot(&stream_var_snapshot_);
}

void StreamVar::CaptureStreamVarSnapshot(std::unordered_map<std::string, uint64_t>* snapshot) {
  std::string var_path{""};
  uint64_t var_value{0};

  var_value = GetRpcCallCountValue(&var_path);
  (*snapshot)[var_path] = var_value;

  var_value = GetRpcCallFailureCountValue(&var_path);
  (*snapshot)[var_path] = var_value;

  var_value = GetSendMessageCountValue(&var_path);
  (*snapshot)[var_path] = var_value;

  var_value = GetRecvMessageCountValue(&var_path);
  (*snapshot)[var_path] = var_value;

  var_value = GetSendMessageBytesValue(&var_path);
  (*snapshot)[var_path] = var_value;

  var_value = GetRecvMessageBytesValue(&var_path);
  (*snapshot)[var_path] = var_value;
}

void StreamVar::GetStreamVarMetrics(std::unordered_map<std::string, uint64_t>* metrics) {
  CaptureStreamVarSnapshot(metrics);

  for (const auto& [var_path, var_value] : *metrics) {
    auto snapshot_value = var_value;
    (*metrics)[var_path] -= stream_var_snapshot_[var_path];
    stream_var_snapshot_[var_path] = snapshot_value;
  }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

StreamVarHelper::StreamVarHelper() : active_stream_count_("trpc/stream_rpc/active_stream_count") {}

StreamVarHelper::~StreamVarHelper() { stream_vars_.clear(); }

StreamVarPtr StreamVarHelper::GetOrCreateStreamVar(const std::string& var_path) {
  auto found = stream_vars_.find(var_path);
  if (found != stream_vars_.end()) {
    return found->second;
  }

  // FIXME: slow mutex.
  std::unique_lock lock(mutex_);
  found = stream_vars_.find(var_path);
  if (found == stream_vars_.end()) {
    auto stream_var = CreateStreamVar(var_path);
    stream_vars_.emplace(var_path, stream_var);
    return stream_var;
  }

  return found->second;
}

bool StreamVarHelper::ReportStreamVarMetrics(const std::string& metrics_name) {
  std::unique_lock lock_r(mutex_);
  auto stream_var_reader = stream_vars_;
  lock_r.unlock();

  for (const auto& it : stream_var_reader) {
    std::unordered_map<std::string, uint64_t> metrics;
    it.second->GetStreamVarMetrics(&metrics);
    ReportMetrics(metrics_name, metrics);
    metrics.clear();
  }

  return (!stream_var_reader.empty());
}

bool StreamVarHelper::ReportMetrics(const std::string& metrics_name,
                                    const std::unordered_map<std::string, uint64_t>& metrics) {
  auto metrics_ptr = MetricsFactory::GetInstance()->Get(metrics_name);

  if (!metrics_ptr) {
    return false;
  }

  for (const auto& [var_path, var_value] : metrics) {
    SingleAttrMetricsInfo single_attr_info;
    single_attr_info.name = kStreamRuntimeInfo;
    single_attr_info.dimension = var_path;
    single_attr_info.value = static_cast<double>(var_value);
    single_attr_info.policy = MetricsPolicy::SUM;
    if (metrics_ptr->SingleAttrReport(single_attr_info) != 0) {
      TRPC_FMT_TRACE("trpc stream var metrics report failed, dimension:{} value:{}", single_attr_info.dimension,
                     single_attr_info.value);
      return false;
    }
    TRPC_FMT_TRACE("trpc stream var metrics report, dimension:{} value:{}", single_attr_info.dimension,
                   single_attr_info.value);
  }

  return true;
}

void StreamVarHelper::IncrementActiveStreamCount() { active_stream_count_.Increment(); }

void StreamVarHelper::DecrementActiveStreamCount() { active_stream_count_.Decrement(); }

uint64_t StreamVarHelper::GetActiveStreamCount() { return active_stream_count_.GetValue(); }

}  // namespace trpc::stream
