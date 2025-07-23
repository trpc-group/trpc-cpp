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

#pragma once

#include <cstdint>
#include <string>

#include "trpc/util/chrono/time.h"

namespace trpc {

/// @brief Get the number of nanoseconds from the Epoch to now.
uint64_t GetSystemNanoSeconds();

/// @brief Get the number of microseconds from the Epoch to now.
uint64_t GetSystemMicroSeconds();

/// @brief Get the number of milliseconds from the Epoch to now.
uint64_t GetSystemMilliSeconds();

/// @brief Get the number of nanoseconds from system boot to now.
uint64_t GetSteadyNanoSeconds();

/// @brief Get the number of microseconds from system boot to now.
uint64_t GetSteadyMicroSeconds();

/// @brief Get the number of milliseconds from system boot to now.
uint64_t GetSteadyMilliSeconds();

/// @brief Get timestamp in nanoseconds. We use it to set timestamp during each progress. You can use it to calculate
/// interval from which we set.
/// @note Use steady_clock by default.
uint64_t GetNanoSeconds();

/// @brief Get timestamp in microseconds. We use it to set timestamp during each progress. You can use it to calculate
/// interval from which we set.
/// @note Use steady_clock by default.
uint64_t GetMicroSeconds();

/// @brief Get timestamp in millseconds. We use it to set timestamp during each progress. You can use it to calculate
/// interval from which we set.
/// @note Use steady_clock by default.
uint64_t GetMilliSeconds();

/// @brief Get the number of seconds from the Epoch to now in time_t struct.
time_t GetNowAsTimeT();

/// @brief Get the milliseconds from epoch in timeval struct.
timeval GetNowAsTimeVal();

class TimeProvider {
 public:
  /// @brief Get the number of seconds from the Epoch to now.
  /// @private
  [[deprecated("Use trpc::time::GetNowAsTimeT() instead")]]
  static time_t GetNow();

  /// @brief Get the number of milliseconds from system boot to now.
  /// @private
  [[deprecated("Use trpc::time::GetMilliSeconds() instead")]]
  static uint64_t GetNowMs();

  /// @brief Get the number of microseconds from system boot to now.
  /// @private
  [[deprecated("Use trpc::time::GetMicroSeconds() instead")]]
  static uint64_t GetNowUs();

  /// @brief Get the milliseconds from epoch and store in tv.
  /// @private
  [[deprecated("Use trpc::time::GetNowAsTimeVal() instead")]]
  static void GetNow(timeval* tv);
};

class TimeStringHelper {
 public:
  template <typename... Args>
  static std::string StringFormat(const std::string& format, Args... args) {
    return time::StringFormat(format, args...);
  }
  /// @brief Convert Unix time to a string.
  /// @param unix_time time in time_t type
  /// @return time in string type
  static std::string ConvertUnixTimeToStr(time_t unix_time);

  /// @brief Convert milliseconds timestamp to string.
  /// @param ms milliseconds timestamp
  /// @return time in string type, e.g. 2023-03-16 18:01:29:426345
  static std::string ConvertMillSecsToStr(int64_t ms);

  /// @brief Convert microseconds timestamp to string.
  /// @param us microseconds timestamp
  /// @return time in string type, e.g. 2021-12-16 18:01:29:426345
  static std::string ConvertMicroSecsToStr(int64_t us);

  /// @brief Convert epoch (time_t) time to HTTP Date format string type.
  /// @param time time in time_t type
  /// @return time in HTTP Date format string type, e.g., Mon, 10 Oct 2016 10:25:58 GMT
  static std::string ConvertEpochToHttpDate(time_t time);

  /// @brief Convert time in HTTP Date format string type to epoch (time_t) time.
  static time_t ConvertHttpDateToEpoch(const std::string& http_date);

  /// @brief Convert tm time to time_t time
  static time_t ConvertTmToTimeGm(struct tm* tm);

  /// @brief Convert tm time to time_t time without using tm->tm_yday
  /// @note  This method works well in conjunction with strptime(), as some platforms do not calculate tm_yday when
  ///        calling strptime().
  static time_t ConvertTmToTimeGmWithoutYDay(struct tm* tm);
};

}  // namespace trpc
