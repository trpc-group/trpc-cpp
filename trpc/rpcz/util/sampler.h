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

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#pragma once

#include <atomic>
#include <memory>
#include <string>

namespace trpc::rpcz {

/// @brief To limit sampling rate, window size is 1s.
/// @private
class SampleRateLimiter {
 private:
  /// @brief Record the count and timestamp of current sampling state.
  union SampleRateLimiterStat {
    SampleRateLimiterStat() {}

    std::atomic<uint64_t> u64;
    struct {
      std::atomic<uint32_t> timestamp;
      std::atomic<uint32_t> count;
    } fields;
  };

 public:
  /// @brief Constructor.
  SampleRateLimiter() = default;

  /// @brief Destructor.
  ~SampleRateLimiter() = default;

  /// @brief To init the stat.
  void Init();

  /// @brief Increase count by 1 if in same window.
  uint32_t Increase();

  /// @brief Get sample count in current window.
  uint32_t GetSampleNum();

 private:
  /// @brief Move window if neccesary.
  void SyncCheck();

 private:
  std::unique_ptr<SampleRateLimiterStat> sample_rate_limiter_stat_;
};

/// @brief Considering sample count in current window:
///        [0, low water), all sampled.
///        [low water, high water), limit sample rate.
///        [high water, ~), stop sample.
/// @private
class HighLowWaterLevelSampler {
 public:
  /// @brief Default constructor.
  HighLowWaterLevelSampler() = default;

  /// @brief Default destructor.
  ~HighLowWaterLevelSampler() = default;

  /// @brief Init sampler.
  void Init();

  /// @brief Get sampler name.
  std::string GetName() const { return "HighLowWaterLevelSampler"; }

  /// @brief Determine whether to sample or not.
  /// @param id Span id.
  /// @return True, should sampled, false, not sampled.
  /// @note Here, sampling is done based on the size of the span_id. After adding user-defined span objects,
  ///       the latter does not go through this sampling process. However, the span ID is still obtained
  ///       globally, which can interfere with the sampling process here, causing inaccuracies in the
  ///       sampling of framework rpcz when both framework and user-defined rpcz are used simultaneously.
  ///       Anyway, the effectiveness of sampling still exists.
  bool Sample(uint32_t id) {
    // Hitting sample rate.
    if (sampling_rate_ > 0 && id % sampling_rate_ == 0) {
      // But current window bigger than high water, stop sample.
      if (sample_rate_limiter_.Increase() > speed_max_rate_) {
        return false;
      } else {
        // Otherwise, do sample.
        return true;
      }
    } else {
      // Not hitting sample rate, but current window count is less than low water, sample anyway.
      if (sample_rate_limiter_.GetSampleNum() < speed_min_rate_) {
        sample_rate_limiter_.Increase();
        return true;
      } else {
        // Otherwise, stop sample.
        return false;
      }
    }
  }

 private:
  // Sample rate, may be 0.
  uint32_t sampling_rate_;
  // Low water.
  uint32_t speed_min_rate_;
  // High water.
  uint32_t speed_max_rate_;
  // Limiter by window.
  SampleRateLimiter sample_rate_limiter_;
};

}  // trpc::rpcz
#endif
