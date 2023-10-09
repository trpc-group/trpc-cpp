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

#include <sys/prctl.h>

#include <chrono>
#include <ctime>
#include <memory>
#include <ratio>
#include <string_view>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "spdlog/details/os.h"
#include "spdlog/sinks/sink.h"

#include "trpc/util/log/default/default_log.h"
#include "trpc/util/log/default/sinks/local_file/hourly_file_sink.h"

namespace trpc::testing {

using HourlyFileSinkPtr = trpc::HourlyFileSink<std::mutex, trpc::HourlyFilenameCalculator>;
using LogSystemClock = std::chrono::_V2::system_clock;
using HoursType = std::chrono::duration<int, std::ratio<60 * 60>>;

class HourlyFileSinkTest : public HourlyFileSinkPtr {
 public:
  explicit HourlyFileSinkTest(uint16_t hour_interval = 1) : HourlyFileSinkPtr("default.log", false, 2, hour_interval) {}

  LogSystemClock::time_point WrapNextRotationTimePoint() { return NextRotationTimePoint(); }

  std::tm GetTm() {
    auto now = LogSystemClock::now();
    return NowTime(now);
  }

  size_t WrapGetFileNamesQueueSize() { return GetFileNamesQueueSize(); }

  void WrapDeleteOldFile() { DeleteOldFile(); }

  uint16_t WrapGetHourInterval() { return GetHourInterval(); }

  const spdlog::filename_t& WrapFileName() const { return FileName(); }

  void WrapSinkIt(const spdlog::details::log_msg& msg) { sink_it_(msg); }
};

using hourly_sink_ptr = std::shared_ptr<HourlyFileSinkTest>;

TEST(HourlyFileSinkTest, test_GetHourInterval) {
  // Interval time range（0-23）
  hourly_sink_ptr hourly_sink(new HourlyFileSinkTest(2));
  ASSERT_EQ(hourly_sink->WrapGetHourInterval(), 2);
}

TEST(HourlyFileSinkTest, test_filename) {
  hourly_sink_ptr hourly_sink(new HourlyFileSinkTest());
  auto filename = trpc::HourlyFilenameCalculator::CalcFileName("default.log", hourly_sink->GetTm());
  std::cout << "filename:" << filename << std::endl;
  ASSERT_EQ(hourly_sink->WrapFileName(), filename);
}

TEST(HourlyFileSinkTest, test_NextRotationTimePoint) {
  hourly_sink_ptr hourly_sink(new HourlyFileSinkTest());

  LogSystemClock::time_point next_point = hourly_sink->WrapNextRotationTimePoint();
  LogSystemClock::time_point current_point = std::chrono::time_point_cast<HoursType>(LogSystemClock::now());
  ASSERT_EQ(std::chrono::duration_cast<std::chrono::hours>(next_point.time_since_epoch()).count() -
                std::chrono::duration_cast<std::chrono::hours>(current_point.time_since_epoch()).count(),
            1);
}

TEST(HourlyFileSinkTest, test_delete_old_) {
  hourly_sink_ptr hourly_sink(new HourlyFileSinkTest());
  spdlog::level::level_enum spd_level =
      static_cast<spdlog::level::level_enum>(static_cast<int>(trpc::Log::Level::debug));
  spdlog::details::log_msg log_msg(spdlog::source_loc{"xxx.cc", 1, "sink_it"}, "log_default_default", spd_level,
                                   spdlog::string_view_t("testing hourly", 10));
  hourly_sink->WrapSinkIt(log_msg);
  // Set the Max number of files to 2 and 1 before calling delete function
  ASSERT_EQ(hourly_sink->WrapGetFileNamesQueueSize(), 1);

  hourly_sink->WrapDeleteOldFile();

  // After the delete function, another file will be added because the Max number of files is not reached
  ASSERT_EQ(hourly_sink->WrapGetFileNamesQueueSize(), 2);

  hourly_sink->WrapDeleteOldFile();

  // When the maximum number of files is reached, the old files will be deleted first,
  // and then a new one will be added, always keeping the maximum number of files set.
  ASSERT_EQ(hourly_sink->WrapGetFileNamesQueueSize(), 2);
}

}  // namespace trpc::testing
