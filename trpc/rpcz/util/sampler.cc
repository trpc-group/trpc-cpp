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

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#include "trpc/rpcz/util/sampler.h"

#include <cstring>
#include <stdlib.h>
#include <time.h>

#include "trpc/common/config/trpc_config.h"

namespace trpc::rpcz {

void HighLowWaterLevelSampler::Init() {
  sampling_rate_ = trpc::TrpcConfig::GetInstance()->GetGlobalConfig().rpcz_config.sample_rate;
  speed_min_rate_ = trpc::TrpcConfig::GetInstance()->GetGlobalConfig().rpcz_config.lower_water_level;
  speed_max_rate_ = trpc::TrpcConfig::GetInstance()->GetGlobalConfig().rpcz_config.high_water_level;
  sample_rate_limiter_.Init();
}

void SampleRateLimiter::Init() {
  sample_rate_limiter_stat_ = std::make_unique<SampleRateLimiterStat>();
  memset(reinterpret_cast<void*>(sample_rate_limiter_stat_.get()), 0, sizeof(SampleRateLimiterStat));
}

uint32_t SampleRateLimiter::Increase() {
  if (!sample_rate_limiter_stat_) return 0;

  this->SyncCheck();
  sample_rate_limiter_stat_->fields.count.fetch_add(1, std::memory_order_relaxed);
  return sample_rate_limiter_stat_->fields.count.load(std::memory_order_acquire);
}

uint32_t SampleRateLimiter::GetSampleNum() {
  if (!sample_rate_limiter_stat_) return 0;

  this->SyncCheck();
  return sample_rate_limiter_stat_->fields.count.fetch_add(0, std::memory_order_relaxed);
}

void SampleRateLimiter::SyncCheck() {
  uint32_t now = static_cast<uint32_t>(time(nullptr));
  SampleRateLimiterStat old_val;
  old_val.u64.store(sample_rate_limiter_stat_->u64, std::memory_order_release);
  // Window not expired yet.
  if (old_val.fields.timestamp.load(std::memory_order_acquire) == now)
    return;

  /// @note Pretty rough window.
  SampleRateLimiterStat new_val;
  new_val.fields.timestamp.store(now, std::memory_order_release);
  new_val.fields.count.store(0, std::memory_order_release);

  auto old_value = old_val.u64.load(std::memory_order_acquire);
  auto new_value = new_val.u64.load(std::memory_order_acquire);
  sample_rate_limiter_stat_->u64.compare_exchange_strong(old_value, new_value, std::memory_order_acquire);
}

}  // namespace trpc::rpcz
#endif
