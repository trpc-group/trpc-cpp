//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2024 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL
#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include "trpc/util/log/logging.h"

/// The entire queue did not achieve strict synchronization
/// Ensure the atomicity of single step operations through atomic variables Select memory sequence for optimization
/// When calling the checkpoint() of this queue, a double loop is used to ensure that the threshold is not exceeded for
/// a moment

namespace trpc::overload_control {
class RequestRollQueue {
 public:
  explicit RequestRollQueue(int n_fps);

  virtual ~RequestRollQueue();

  /// @brief move windows
  void NextTimeFrame();

  /// @brief Add time count
  int64_t AddTimeHit();

  /// @brief Get the total_size of the smooth window
  int64_t ActiveSum() const;

 private:
  int WindowSize() const { return time_hits_.size() - kRedundantFramesNum; }

  // begin + 1
  int NextIndex(int idx) const {
    return isPowOfTwo_ ? (idx + 1) & (time_hits_.size() - 1) : (idx + 1) % time_hits_.size();
  }

 private:
  // Record the starting position
  std::atomic<int> begin_pos_ = 0;

  std::atomic<int> current_limit_ = 0;

  // Time slot queue
  std::vector<std::atomic<int64_t>> time_hits_;

  // Redundant positions ,When judging if the team is empty or full
  static constexpr int kRedundantFramesNum = 1;

  // Determine whether the residual operation can be optimized
  bool isPowOfTwo_;
};
}  // namespace trpc::overload_control
#endif