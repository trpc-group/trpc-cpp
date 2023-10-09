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
// 1. nghttp2
// Copyright (c) 2013 Tatsuhiro Tsujikawa
// nghttp2 is licensed under the MIT License.
//
//

#include "trpc/util/chrono/time.h"

#include <sys/time.h>

#include "trpc/util/chrono/chrono.h"

namespace trpc::time {

uint64_t GetSystemMilliSeconds() {
  return std::chrono::time_point_cast<std::chrono::milliseconds>(ReadSystemClock()).time_since_epoch().count();
}

uint64_t GetSystemMicroSeconds() {
  return std::chrono::time_point_cast<std::chrono::microseconds>(ReadSystemClock()).time_since_epoch().count();
}

uint64_t GetSystemNanoSeconds() {
  return std::chrono::time_point_cast<std::chrono::nanoseconds>(ReadSystemClock()).time_since_epoch().count();
}

uint64_t GetSteadyMilliSeconds() {
  return std::chrono::time_point_cast<std::chrono::milliseconds>(ReadSteadyClock()).time_since_epoch().count();
}

uint64_t GetSteadyMicroSeconds() {
  return std::chrono::time_point_cast<std::chrono::microseconds>(ReadSteadyClock()).time_since_epoch().count();
}

uint64_t GetSteadyNanoSeconds() {
  return std::chrono::time_point_cast<std::chrono::nanoseconds>(ReadSteadyClock()).time_since_epoch().count();
}

uint64_t GetNanoSeconds() { return GetSteadyNanoSeconds(); }

uint64_t GetMicroSeconds() { return GetSteadyMicroSeconds(); }

uint64_t GetMilliSeconds() { return GetSteadyMilliSeconds(); }

time_t GetNowAsTimeT() { return std::chrono::system_clock::to_time_t(ReadSystemClock()); }

timeval GetNowAsTimeVal() {
  timeval tv;
  uint64_t us = GetSystemMicroSeconds();
  tv.tv_sec = us / 1000000;
  tv.tv_usec = us % 1000000;
  return tv;
}

std::string ConvertUnixTimeToStr(time_t unix_time) {
  struct tm* now_time = localtime(&unix_time);
  return StringFormat("%04d-%02d-%02d %02d:%02d:%02d", now_time->tm_year + 1900, now_time->tm_mon + 1,
                      now_time->tm_mday, now_time->tm_hour, now_time->tm_min, now_time->tm_sec);
}

std::string ConvertMillSecsToStr(int64_t ms) {
  int64_t secs = ms / 1000;
  int64_t mills = ms % 1000;
  return StringFormat("%s:%s", ConvertUnixTimeToStr(secs).c_str(), std::to_string(mills).c_str());
}

std::string ConvertMicroSecsToStr(int64_t microSecs) {
  int64_t secs = microSecs / 1000000;
  int64_t micros = microSecs % 1000000;
  return StringFormat("%s:%s", ConvertUnixTimeToStr(secs).c_str(), StringFormat("%06d", micros).c_str());
}

// The following source codes are from nghttp2.
// Copied and modified from
// https://github.com/nghttp2/nghttp2/blob/master/src/timegm.c

namespace {
constexpr const char* kMonthTexts[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
constexpr const char* kDayOfWeekTexts[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
}  // namespace

std::string ConvertEpochToHttpDate(time_t time) {
  struct tm now_time {};
  if (gmtime_r(&time, &now_time) == nullptr) {
    return {29, '\0'};
  }
  return StringFormat("%s, %02d %s %04d %02d:%02d:%02d %s", kDayOfWeekTexts[now_time.tm_wday], now_time.tm_mday,
                      kMonthTexts[now_time.tm_mon], now_time.tm_year + 1900, now_time.tm_hour, now_time.tm_min,
                      now_time.tm_sec, "GMT");
}

time_t ConvertHttpDateToEpoch(const std::string& http_date) {
  struct tm tm {};
  char* r = strptime(http_date.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &tm);
  if (r == nullptr) {
    return 0;
  }
  return ConvertTmToTimeGmWithoutYDay(&tm);
}

namespace {
/// @brief Calculate the number of leap years within a range of [0, year).
/// @param year Century year. (e.g., 2012)
int CountLeapYear(int year) {
  year -= 1;
  return year / 4 - year / 100 + year / 400;
}

/// @brief Check if a year is a leap year.
/// @param year Century year. (e.g., 2012)
bool IsLeapYear(int year) { return (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0); }

// Number of days before the start of each month.
constexpr int kDaysSum[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
}  // namespace

time_t ConvertTmToTimeGm(struct tm* tm) {
  if (tm->tm_mon > 11) {
    return -1;
  }

  int leap_year_count = CountLeapYear(tm->tm_year + 1900) - CountLeapYear(1970);
  int days = (tm->tm_year - 70) * 365 + leap_year_count + tm->tm_yday;
  int64_t t = (static_cast<int64_t>(days) * 24 + tm->tm_hour) * 3600 + tm->tm_min * 60 + tm->tm_sec;

  return static_cast<time_t>(t);
}

time_t ConvertTmToTimeGmWithoutYDay(struct tm* tm) {
  if (tm->tm_mon > 11) {
    return -1;
  }

  int leap_year_count = CountLeapYear(tm->tm_year + 1900) - CountLeapYear(1970);
  int days = (tm->tm_year - 70) * 365 + leap_year_count + kDaysSum[tm->tm_mon] + tm->tm_mday - 1;
  if (tm->tm_mon >= 2 && IsLeapYear(tm->tm_year + 1900)) {
    ++days;
  }

  int64_t t = (static_cast<int64_t>(days) * 24 + tm->tm_hour) * 3600 + tm->tm_min * 60 + tm->tm_sec;

  return static_cast<time_t>(t);
}
// End of source codes that are from nghttp2.

}  // namespace trpc::time
