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

/// @brief The framework provides interfaces for obtaining various types of time.
/// 1. Obtain the precise time from Epoch or the system start-up time;
///    used in scenarios with high time precision requirements.
/// 2. Obtain the rough timestamp from Epoch using thread sampling; used in scenarios with high performance requirements
///    but low time precision requirements.
/// 3. Conversion between time strings and time.
/// The time interfaces include: milliseconds (ms), microseconds (us), and nanoseconds (ns); 1ms=1000us=1000000ns
/// Epoch: 1970-01-01T00:00:00Z

#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace trpc::time {

/// @brief Obtain the number of nanoseconds from Epoch to the present time,
/// @return uint64_t type, representing a number of nanoseconds.
uint64_t GetSystemNanoSeconds();

/// @brief Obtain the number of microseconds from Epoch to the present time.
/// @return uint64_t type, representing a number of microseconds.
uint64_t GetSystemMicroSeconds();

/// @brief Obtain the number of milliseconds from Epoch to the present time.
/// @return uint64_t type, representing a number of milliseconds.
uint64_t GetSystemMilliSeconds();

/// @brief Obtain the number of nanoseconds from system startup to the present time.
/// @return uint64_t type, representing a number of nanoseconds.
uint64_t GetSteadyNanoSeconds();

/// @brief Obtain the number of microseconds from system startup to the present time.
/// @return uint64_t type, representing a number of microseconds.
uint64_t GetSteadyMicroSeconds();

/// @brief Obtain the number of milliseconds from system startup to the present time.
/// @return uint64_t type, representing a number of milliseconds.
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

/// @brief Obtaining the string representation of time.
/// @tparam ...Args Variadic parameter type.
/// @param format String format.
/// @param ...args Variadic parameters, which are actual data used for formatting.
/// @return String that stores time.
template <typename... Args>
std::string StringFormat(const std::string& format, Args... args) {
  size_t size = snprintf(nullptr, 0, format.c_str(), args...) + 1;  // Extra space for '\0'
  std::unique_ptr<char[]> buf(new char[size]);
  snprintf(buf.get(), size, format.c_str(), args...);
  return {buf.get(), buf.get() + size - 1};  // We don't want the '\0' inside
}

/// @brief Converting Unix time type to string.
/// @param unix_time time_t type, Unix time parameter.
/// @return String that stores time.
std::string ConvertUnixTimeToStr(time_t unix_time);

/// @brief Converting millisecond timestamp to string.
/// @param ms Millisecond timestamp
/// @return String that stores time, such as "2023-03-16 18:01:29:426345".
std::string ConvertMillSecsToStr(int64_t ms);

/// @brief Converting microseconds timestamp to string.
/// @param us Microseconds timestamp
/// @return String that stores time, such as "2023-03-16 18:01:29:426345".
std::string ConvertMicroSecsToStr(int64_t us);

/// @brief Converting Epoch (time_t) time to HTTP Date format string.
/// @param time Time variable of type time_t.
/// @return String that stores time, such as "Mon, 10 Oct 2016 10:25:58 GMT".
std::string ConvertEpochToHttpDate(time_t time);

/// @brief Converting HTTP Date format string to Epoch (time_t) time.
/// @param http_date Time string., e.g., Mon, 10 Oct 2016 10:25:58 GMT
/// @return Time of type time_t.
time_t ConvertHttpDateToEpoch(const std::string& http_date);

/// @brief Converting tm to GM time.
/// @param t Time of type tm.
/// @return Time of type time_t.
time_t ConvertTmToTimeGm(struct tm* t);

/// @brief Converting tm to GM time, but without using tm->tm_yday.
/// @param t Time of type tm.
/// @return Time of type time_t.
/// @note This method is preferable when using strptime(),
///       as some platforms do not calculate tm_yday when calling strptime().
time_t ConvertTmToTimeGmWithoutYDay(struct tm* t);

}  // namespace trpc::time
