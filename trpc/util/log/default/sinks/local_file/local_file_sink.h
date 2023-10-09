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

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <chrono>
#include <ctime>
#include <memory>
#include <ratio>
#include <regex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "spdlog/details/os.h"
#include "spdlog/pattern_formatter.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/version.h"

#include "trpc/common/config/default_log_conf.h"
#include "trpc/common/config/local_file_sink_conf.h"
#include "trpc/util/log/default/sinks/local_file/file_helper.h"
#include "trpc/util/log/default/sinks/local_file/hourly_file_sink.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

class LocalFileSink : public RefCounted<LocalFileSink> {
 public:
  /// @brief Get the sink instance of spdlog
  spdlog::sink_ptr SpdSink() const { return sink_; }

  /// @brief Initializing the sink
  /// @param config Output config
  /// @return int   Return 0 means success and a complex number means failure
  int Init(const LocalFileSinkConfig& config);

 protected:
  /// @brief In daily log mode, delete log files that have expired locally.
  /// @note  Definition of timeout: Under the condition that the number of log files is set to k,
  /// the logs that are prior to k days from the current time need to be deleted.
  /// @param file_name       Filename
  /// @param reserver_count  Logs that are k days old are deleted
  /// @return bool           true/false
  bool RemoveTimeoutDailyLogFile(const std::string& file_name, int reserver_count);

  /// @brief The time information in the filename with the split path is stored in the tm object
  /// @note example:
  ///       "filename = trpc_2021-04-23.log, ext = .log" => ("tm_time information:
  ///       "tm_time.tm_year=2021|tm_time.tm_mon=4|tm_time.tm_mday=23")
  /// @param file_name      Filename
  /// @param ext            File suffix
  /// @param tm_time        Outgoing parameter, which represents time
  /// @return bool          true/false
  bool SplitFileNameTime(const std::string& file_name, const std::string& ext, tm& tm_time);

  // sink plugin localfile configuration
  LocalFileSinkConfig config_;

  // sink for the spdlog library
  spdlog::sink_ptr sink_;
};

using LocalFileSinkPtr = RefPtr<LocalFileSink>;

}  // namespace trpc
