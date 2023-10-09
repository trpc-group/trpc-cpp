//
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/chrono.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/chrono/chrono.h"

#include <time.h>

using namespace std::literals;

namespace trpc {

namespace {

template <class T>
inline T ReadClock(int type) {
  timespec ts;
  clock_gettime(type, &ts);
  return T((ts.tv_sec * 1'000'000'000LL + ts.tv_nsec) * 1ns);
}

}  // namespace

std::chrono::steady_clock::time_point ReadSteadyClock() {
  return ReadClock<std::chrono::steady_clock::time_point>(CLOCK_MONOTONIC);
}

std::chrono::system_clock::time_point ReadSystemClock() {
  return ReadClock<std::chrono::system_clock::time_point>(CLOCK_REALTIME);
}

}  // namespace trpc
