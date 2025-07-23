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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. incubator-brpc
// Copyright (C) 2019 The Apache Software Foundation
// incubator-brpc is licensed under the Apache 2.0 License.
//
//

#include "trpc/tvar/compound_ops/latency_recorder.h"

#include <memory>
#include <string_view>

#include "trpc/common/config/trpc_config.h"

namespace {

using trpc::tvar::PercentileSamples;
using RecorderWindow = trpc::tvar::LatencyRecorder::RecorderWindow;
using PercentileWindow = trpc::tvar::LatencyRecorder::PercentileWindow;
using CombinedPercentileSamples = PercentileSamples<1022>;

inline std::string GetRelPath(std::string_view path) {
  std::string temp_path;
  if (!path.empty()) {
    temp_path.reserve(path.size() + 1);
    temp_path = path;
    if (path.back() != '/') {
      temp_path.append("/");
    }
  }
  return temp_path;
}

}  // namespace

namespace trpc::tvar {

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/bvar/latency_recorder.cpp

LatencyRecorder::LatencyRecorder(time_t window_size)
    : latency_(),
      max_latency_(0),
      latency_percentile_(),
      latency_percentile_window_(&latency_percentile_, window_size),
      rel_path_(),
      latency_window_(&latency_, window_size),
      max_latency_window_(&max_latency_, window_size),
      count_([this]() { return latency_.GetValue().num; }),
      qps_([this]() { return QPS(WindowSize()); }),
      latency_p1_([this]() {
        return LatencyPercentile(
            trpc::TrpcConfig::GetInstance()->GetGlobalConfig().tvar_config.latency_p1 / 100.0);
      }),
      latency_p2_([this]() {
        return LatencyPercentile(
            trpc::TrpcConfig::GetInstance()->GetGlobalConfig().tvar_config.latency_p2 / 100.0);
      }),
      latency_p3_([this]() {
        return LatencyPercentile(
            trpc::TrpcConfig::GetInstance()->GetGlobalConfig().tvar_config.latency_p3 / 100.0);
      }),
      latency_999_([this]() { return LatencyPercentile(0.999); }),
      latency_9999_([this]() { return LatencyPercentile(0.9999); }) {}

LatencyRecorder::LatencyRecorder(TrpcVarGroup* parent, std::string_view rel_path,
                                 time_t window_size)
    : latency_(),
      max_latency_(0),
      latency_percentile_(),
      latency_percentile_window_(&latency_percentile_, window_size),
      rel_path_(GetRelPath(rel_path)),
      latency_window_(parent, rel_path_ + "latency", &latency_, window_size),
      max_latency_window_(parent, rel_path_ + "max_latency", &max_latency_, window_size),
      count_(parent, rel_path_ + "count", [this]() { return latency_.GetValue().num; }),
      qps_(parent, rel_path_ + "qps", [this]() { return QPS(WindowSize()); }),
      latency_p1_(
          parent,
          rel_path_ + "latency_" +
              std::to_string(
                  trpc::TrpcConfig::GetInstance()->GetGlobalConfig().tvar_config.latency_p1),
          [this]() {
            return LatencyPercentile(
                trpc::TrpcConfig::GetInstance()->GetGlobalConfig().tvar_config.latency_p1 / 100.0);
          }),
      latency_p2_(
          parent,
          rel_path_ + "latency_" +
              std::to_string(
                  trpc::TrpcConfig::GetInstance()->GetGlobalConfig().tvar_config.latency_p2),
          [this]() {
            return LatencyPercentile(
                trpc::TrpcConfig::GetInstance()->GetGlobalConfig().tvar_config.latency_p2 / 100.0);
          }),
      latency_p3_(
          parent,
          rel_path_ + "latency_" +
              std::to_string(
                  trpc::TrpcConfig::GetInstance()->GetGlobalConfig().tvar_config.latency_p3),
          [this]() {
            return LatencyPercentile(
                trpc::TrpcConfig::GetInstance()->GetGlobalConfig().tvar_config.latency_p3 / 100.0);
          }),
      latency_999_(parent, rel_path_ + "latency_999",
                   [this]() { return LatencyPercentile(0.999); }),
      latency_9999_(parent, rel_path_ + "latency_9999",
                    [this]() { return LatencyPercentile(0.9999); }) {}

/// @brief Get by latency_window_.
uint32_t LatencyRecorder::QPS(time_t window_size) const {
  Sample<RecorderWindow::ReducerSamplerType::DataType> s;
  latency_window_.GetSpan(window_size, &s);
  if (s.time_us <= 0) {
    return 0;
  }
  // Use float to avoid overflow.
  return static_cast<uint32_t>(round(s.data.num * 1000000.0 / s.time_us));
}

/// @brief Get by latency_percentile_window_.
uint32_t LatencyRecorder::LatencyPercentile(double ratio) const {
  auto cb = CombinedPercentileSamples();
  std::vector<GlobalPercentileSamples> buckets;
  latency_percentile_window_.GetSamples(&buckets);
  cb.CombineOf(buckets.begin(), buckets.end());
  return cb.GetNumber(ratio);
}

/// @brief All compound tvars must expose to get a exposed LatencyRecorder.
bool LatencyRecorder::IsExposed() const {
  return latency_window_.IsExposed() && max_latency_window_.IsExposed() && count_.IsExposed() &&
         qps_.IsExposed() && latency_p1_.IsExposed() && latency_p2_.IsExposed() &&
         latency_p3_.IsExposed() && latency_999_.IsExposed() && latency_9999_.IsExposed();
}

/// @brief Update compound tvars.
void LatencyRecorder::Update(uint32_t latency) {
  latency_.Update(latency);
  max_latency_.Update(latency);
  latency_percentile_.Update(latency);
}

// End of source codes that are from incubator-brpc.

}  // namespace trpc::tvar
