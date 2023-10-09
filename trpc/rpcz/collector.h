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
#pragma once

#include <any>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "trpc/common/config/global_conf.h"
#include "trpc/rpcz/span.h"
#include "trpc/rpcz/util/reducer.h"
#include "trpc/rpcz/util/sampler.h"
#include "trpc/util/log/logging.h"

namespace trpc::rpcz {

/// @brief Bind every list of spans with its sorting class.
/// @private
using PreprocessorSpanMap = std::map<SpanPreprocessor*, std::vector<Span*>>;

/// @brief Span id as key.
/// @private
using SpanIdSpanMap = std::unordered_map<uint32_t, Span*>;

/// @brief Store time as key unit in second.
/// @private
using TimeSpanMap = std::unordered_map<uint32_t, std::vector<Span*>>;

/// @brief Minimal value of cache_expire_interval, to improve performance.
/// @private
constexpr uint32_t kCacheExpireIntervalMin = 10;

/// @brief Default number to print span general info.
/// @private
constexpr uint32_t kDefaultPrintNum = 10;

/// @brief Task to merge thread local spans and clear expired spans.
/// @private
class RpczCollectorTask {
 public:
  /// @brief Default constructor.
  RpczCollectorTask();

  /// @brief Default destructor.
  ~RpczCollectorTask();

  /// @brief Add timer task.
  void Start();

  /// @brief Delete timer task.
  void Stop();

  /// @brief Get local span map with span id as key.
  /// @return SpanIdSpanMap
  const SpanIdSpanMap& GetLocalSpanIdSpanData() {
    std::scoped_lock<std::mutex> lock(mutex_);
    return span_id_spans_;
  }

  /// @brief Get local span map with time as key.
  /// @return TimeSpanMap
  const TimeSpanMap& GetLocalTimeSpanData() {
    std::scoped_lock<std::mutex> lock(mutex_);
    return time_spans_;
  }

 private:
  /// @brief Timer task main logic to merge thread local spans and delete expired spans.
  void Run();

  /// @brief Called by Run to collect local spans.
  void OnCollect(PreprocessorSpanMap& prep_map);

  /// @brief Called by Run to clear local spans.
  void OnRemove();

  /// @brief Convert timestamp in microsecond into minute.
  uint32_t ConvertUsToStoreTime(uint64_t us);

  /// @brief Release resource when process exit.
  void Destroy();

 private:
  // Timer task added or not.
  bool start_;

  // Key is sorting class, value is vector<Span*>.
  PreprocessorSpanMap processor_spans_;

  // Rpcz config.
  trpc::RpczConfig rpcz_config_;

  // Last time collect local spans.
  uint64_t last_collect_time_;

  // Last time remove local spans.
  uint64_t last_remove_time_;

  // Span id to span.
  SpanIdSpanMap span_id_spans_;

  // Store time to spans.
  TimeSpanMap time_spans_;

  // To protect map.
  std::mutex mutex_;

  /// Timer id.
  uint64_t task_id_{0};
};

/// @brief Global unique collector to maintain spans for admin query.
/// @private
class RpczCollector : public Reducer<Span*, CombinerCollected> {
 public:
  /// @brief Global instance.
  static RpczCollector* GetInstance() {
    static RpczCollector instance;
    return &instance;
  }

  /// @brief Disable copy.
  RpczCollector(const RpczCollector&) = delete;

  /// @brief Disable assignment.
  RpczCollector& operator=(const RpczCollector&) = delete;

  /// @brief Used by every thread to write span into thread local memory.
  /// @param data Span data.
  void Submit(std::any data);

  /// @brief Fill rpcz data when admin query comes.
  /// @param [in] params admin query.
  /// @param [out] rpcz_data result.
  /// @return bool true: successfully calculate.
  bool FillRpczData(const std::any& params, std::string& rpcz_data);

  /// @brief Start timer task.
  void Start();

  /// @brief Stop timer task.
  void Stop();

 private:
  /// @brief Default constructor.
  RpczCollector() = default;

  /// @brief Get brief span info.
  /// @param [in] span span object.
  /// @param [out] brief_span_info result.
  void FillServerBriefSpanInfo(const Span& span, std::ostream& brief_span_info);

 private:
  // Timer task.
  RpczCollectorTask collect_task_;
};

}  // namespace trpc::rpcz
#endif
