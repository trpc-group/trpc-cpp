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
// 1. incubator-brpc
// Copyright (C) 2019 The Apache Software Foundation
// incubator-brpc is licensed under the Apache 2.0 License.
//
//

#pragma once

#include <math.h>
#include <string.h>

#include <cstdint>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace trpc::tvar {

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/bvar/detail/series.h

/// @brief Check add operator is affordable for user type.
/// @private
template <typename T, typename Op>
struct ProbablyAddition {
  explicit ProbablyAddition(const Op& op) {
    T res(32);
    op(&res, T(64));
    ok_ = (res == T(96));
  }

  operator bool() const { return ok_; }

 private:
  bool ok_;
};

/// @brief Divide on user type with number.
/// @private
template <typename T, typename Op, typename Enabler = void>
struct DivideOnAddition {
  static void InplaceDivide(T& /*obj*/, const Op&, int /*number*/) {}
};

/// @brief Specialized for float type.
/// @private
template <typename T, typename Op>
struct DivideOnAddition<T, Op, typename std::enable_if_t<std::is_floating_point_v<T>>> {
  static void InplaceDivide(T& obj, const Op& op, int number) {
    static ProbablyAddition<T, Op> probably_add(op);
    if (probably_add) {
      obj /= number;
    }
  }
};

/// @brief Specailized for integral type.
/// @private
template <typename T, typename Op>
struct DivideOnAddition<T, Op, typename std::enable_if_t<std::is_integral_v<T>>> {
  static void InplaceDivide(T& obj, const Op& op, int number) {
    static ProbablyAddition<T, Op> probably_add(op);
    if (probably_add) {
      obj = static_cast<T>(round(obj / static_cast<double>(number)));
    }
  }
};

/// @brief Store series data up to last month.
/// @note Data updated every second.
/// @private
template <typename T, typename Op>
class SeriesBase {
 public:
  SeriesBase() : op_(Op()), mutex_(), n_second_(0), n_minute_(0), n_hour_(0), n_day_(0) {}

  /// @brief This func should be called every second.
  void Append(const T& value) {
    std::unique_lock<std::mutex> lc(mutex_);
    return AppendSecond(value, op_);
  }

  std::unordered_map<std::string, std::vector<T>> GetSeries() const noexcept {
    std::scoped_lock<std::mutex> lc(mutex_);
    const int second_begin = n_second_;
    const int minute_begin = n_minute_;
    const int hour_begin = n_hour_;
    const int day_begin = n_day_;
    mutex_.unlock();

    std::vector<T> second_values;
    second_values.reserve(60);
    for (uint32_t i = 0; i < 60; ++i) {
      auto index = (second_begin + 59 - i) % 60;
      second_values.emplace_back(data_.Second(index));
    }

    std::vector<T> minute_values;
    minute_values.reserve(60);
    for (uint32_t i = 0; i < 60; ++i) {
      auto index = (minute_begin + 59 - i) % 60;
      minute_values.emplace_back(data_.Minute(index));
    }

    std::vector<T> hour_values;
    hour_values.reserve(24);
    for (uint32_t i = 0; i < 24; ++i) {
      auto index = (hour_begin + 23 - i) % 24;
      hour_values.emplace_back(data_.Hour(index));
    }

    std::vector<T> day_values;
    day_values.reserve(30);
    for (uint32_t i = 0; i < 30; ++i) {
      auto index = (day_begin + 29 - i) % 30;
      day_values.emplace_back(data_.Day(index));
    }

    std::unordered_map<std::string, std::vector<T>> ret;
    ret.reserve(5);
    ret.emplace("now", std::vector<T>{data_.Second((second_begin + 59) % 60)});
    ret.emplace("latest_sec", std::move(second_values));
    ret.emplace("latest_min", std::move(minute_values));
    ret.emplace("latest_hour", std::move(hour_values));
    ret.emplace("latest_day", std::move(day_values));
    return ret;
  }

 private:
  void AppendSecond(const T& value, const Op& op);
  void AppendMinute(const T& value, const Op& op);
  void AppendHour(const T& value, const Op& op);
  void AppendDay(const T& value);

  struct Data {
   public:
    Data() {
      if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
        memset(array_, 0, sizeof(array_));
      }
    }

    T& Second(int index) { return array_[index]; }
    const T& Second(int index) const { return array_[index]; }

    T& Minute(int index) { return array_[60 + index]; }
    const T& Minute(int index) const { return array_[60 + index]; }

    T& Hour(int index) { return array_[120 + index]; }
    const T& Hour(int index) const { return array_[120 + index]; }

    T& Day(int index) { return array_[144 + index]; }
    const T& Day(int index) const { return array_[144 + index]; }

   private:
    T array_[60 + 60 + 24 + 30];
  };

 protected:
  // For better performance, no need to initailize an instance every time calling operator.
  Op op_;
  mutable std::mutex mutex_;
  uint8_t n_second_;
  uint8_t n_minute_;
  uint8_t n_hour_;
  uint8_t n_day_;
  Data data_{};
};

/// @brief Insert last second data.
/// @private
template <typename T, typename Op>
void SeriesBase<T, Op>::AppendSecond(const T& value, const Op& op) {
  data_.Second(n_second_) = value;
  ++n_second_;
  if (n_second_ >= 60) {
    n_second_ = 0;
    T tmp = data_.Second(0);
    for (int i = 1; i < 60; ++i) {
      op(&tmp, data_.Second(i));
    }
    DivideOnAddition<T, Op>::InplaceDivide(tmp, op, 60);
    AppendMinute(tmp, op);
  }
}

/// @brief Insert last minute daa.
template <typename T, typename Op>
void SeriesBase<T, Op>::AppendMinute(const T& value, const Op& op) {
  data_.Minute(n_minute_) = value;
  ++n_minute_;
  if (n_minute_ >= 60) {
    n_minute_ = 0;
    T tmp = data_.Minute(0);
    for (int i = 1; i < 60; ++i) {
      op(&tmp, data_.Minute(i));
    }
    DivideOnAddition<T, Op>::InplaceDivide(tmp, op, 60);
    AppendHour(tmp, op);
  }
}

/// @brief Insert last hour data.
template <typename T, typename Op>
void SeriesBase<T, Op>::AppendHour(const T& value, const Op& op) {
  data_.Hour(n_hour_) = value;
  ++n_hour_;
  if (n_hour_ >= 24) {
    n_hour_ = 0;
    T tmp = data_.Hour(0);
    for (int i = 1; i < 24; ++i) {
      op(&tmp, data_.Hour(i));
    }
    DivideOnAddition<T, Op>::InplaceDivide(tmp, op, 24);
    AppendDay(tmp);
  }
}

/// @brief Insert last day data.
/// @note Data older than 30 days will be covered.
template <typename T, typename Op>
void SeriesBase<T, Op>::AppendDay(const T& value) {
  data_.Day(n_day_) = value;
  ++n_day_;
  if (n_day_ >= 30) {
    n_day_ = 0;
  }
}

// End of source codes that are from incubator-brpc.

/// @brief Helper for SeriesBase.
/// @private
template <typename T, typename Op>
using Series = SeriesBase<T, Op>;

}  // namespace trpc::tvar
