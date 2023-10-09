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

#include "trpc/util/time.h"

namespace trpc {

uint64_t GetSystemNanoSeconds() { return time::GetSystemNanoSeconds(); }

uint64_t GetSystemMicroSeconds() { return time::GetSystemMicroSeconds(); }

uint64_t GetSystemMilliSeconds() { return time::GetSystemMilliSeconds(); }

uint64_t GetSteadyNanoSeconds() { return time::GetSteadyNanoSeconds(); }

uint64_t GetSteadyMicroSeconds() { return time::GetSteadyMicroSeconds(); }

uint64_t GetSteadyMilliSeconds() { return time::GetSteadyMilliSeconds(); }

uint64_t GetNanoSeconds() { return time::GetNanoSeconds(); }

uint64_t GetMicroSeconds() { return time::GetMicroSeconds(); }

uint64_t GetMilliSeconds() { return time::GetMilliSeconds(); }

time_t GetNowAsTimeT() { return time::GetNowAsTimeT(); }

timeval GetNowAsTimeVal() { return time::GetNowAsTimeVal(); }

time_t TimeProvider::GetNow() { return time::GetNowAsTimeT(); }

uint64_t TimeProvider::GetNowMs() { return time::GetSteadyMilliSeconds(); }

uint64_t TimeProvider::GetNowUs() { return time::GetSteadyMicroSeconds(); }

void TimeProvider::GetNow(timeval* tv) {
  timeval tv_gen = time::GetNowAsTimeVal();
  *tv = std::move(tv_gen);
}

std::string TimeStringHelper::ConvertUnixTimeToStr(time_t unix_time) { return time::ConvertUnixTimeToStr(unix_time); }

std::string TimeStringHelper::ConvertMillSecsToStr(int64_t ms) { return time::ConvertMillSecsToStr(ms); }

std::string TimeStringHelper::ConvertMicroSecsToStr(int64_t us) { return time::ConvertMicroSecsToStr(us); }

std::string TimeStringHelper::ConvertEpochToHttpDate(time_t time) { return time::ConvertEpochToHttpDate(time); }

time_t TimeStringHelper::ConvertHttpDateToEpoch(const std::string& http_date) {
  return time::ConvertHttpDateToEpoch(http_date);
}

time_t TimeStringHelper::ConvertTmToTimeGm(struct tm* tm) { return time::ConvertTmToTimeGm(tm); }

time_t TimeStringHelper::ConvertTmToTimeGmWithoutYDay(struct tm* tm) { return time::ConvertTmToTimeGmWithoutYDay(tm); }

}  // namespace trpc
