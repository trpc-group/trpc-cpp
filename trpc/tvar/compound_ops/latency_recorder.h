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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. incubator-brpc
// Copyright (C) 2019 The Apache Software Foundation
// incubator-brpc is licensed under the Apache 2.0 License.
//
//

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "trpc/tvar/basic_ops/passive_status.h"
#include "trpc/tvar/basic_ops/recorder.h"
#include "trpc/tvar/basic_ops/reducer.h"
#include "trpc/tvar/common/percentile.h"
#include "trpc/tvar/common/tvar_group.h"
#include "trpc/tvar/compound_ops/window.h"

namespace trpc::tvar {

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/bvar/latency_recorder.h

/// @brief Use multiple tvars to record latency.
class LatencyRecorder {
 public:
  /// @private
  using RecorderWindow = Window<IntRecorder, SeriesFrequency::SERIES_IN_SECOND>;
  /// @private
  using MaxWindow = Window<Maxer<uint32_t>, SeriesFrequency::SERIES_IN_SECOND>;
  /// @private
  using PercentileWindow = Window<WriteMostlyPercentile, SeriesFrequency::SERIES_IN_SECOND>;

 public:
  /// @brief Use tvar window size in config as default value.
  explicit LatencyRecorder(time_t window_size = -1);

  LatencyRecorder(TrpcVarGroup* parent, std::string_view rel_path, time_t window_size = -1);

  explicit LatencyRecorder(std::string_view rel_path, time_t window_size = -1)
      : LatencyRecorder(TrpcVarGroup::FindOrCreate("/"), rel_path, window_size) {}

  time_t WindowSize() const { return latency_window_.WindowSize(); }

  void Update(uint32_t latency);

  /// @brief Get average latency in latest window.
  uint32_t Latency(time_t window_size) const {
    return latency_window_.GetValue(window_size).Average();
  }

  /// @brief Get average latency in latest default window.
  uint32_t Latency() const { return latency_window_.GetValue().Average(); }

  /// @brief Get max latency in current window.
  uint32_t MaxLatency() const { return max_latency_window_.GetValue(); }

  uint32_t Count() const { return latency_.GetValue().num; }

  /// @brief Get qps in last window.
  uint32_t QPS(time_t window_size) const;

  /// @brief Get Qps in last second.
  uint32_t QPS() const { return qps_.GetValue(); }

  /// @brief Eg. get 99% latency in current window.
  uint32_t LatencyPercentile(double ratio) const;

  bool IsExposed() const;

 private:
  IntRecorder latency_;
  Maxer<uint32_t> max_latency_;
  WriteMostlyPercentile latency_percentile_;
  PercentileWindow latency_percentile_window_;

 private:
  std::string rel_path_;
  RecorderWindow latency_window_;
  MaxWindow max_latency_window_;
  PassiveStatus<uint32_t> count_;
  PassiveStatus<uint32_t> qps_;
  PassiveStatus<uint32_t> latency_p1_;
  PassiveStatus<uint32_t> latency_p2_;
  PassiveStatus<uint32_t> latency_p3_;
  PassiveStatus<uint32_t> latency_999_;
  PassiveStatus<uint32_t> latency_9999_;
};

// End of source codes that are from incubator-brpc.

}  // namespace trpc::tvar
