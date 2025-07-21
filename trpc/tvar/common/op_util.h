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

#include <sys/time.h>

#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "json/json.h"

#include "trpc/util/log/logging.h"

namespace trpc::tvar {

/// @brief Add const & prefix to non integral type, to improve parameter passing performance.
/// @private
template <typename T>
struct add_cr_non_integral {
  using type = std::conditional_t<std::is_integral_v<T>, T, std::add_lvalue_reference_t<std::add_const_t<T>>>;
};

/// @brief Helper for add_cr_non_integral.
/// @private
template <typename T>
using add_cr_non_integral_t = typename add_cr_non_integral<T>::type;

/// @brief Represent non supported operation.
/// @private
struct VoidOp {
  template <typename T>
  T operator()(T*, const T&) const {
    TRPC_ASSERT(false && "This function should never be called, abort");
  }
};

/// @brief Helper for VoidOp.
/// @private
template <typename T>
using is_void_op = std::is_same<T, VoidOp>;

/// @brief Get system time in micro second unit.
/// @private
inline uint64_t GetSystemMicroseconds() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

/// @brief Add two value typed by T, and store result in first argument.
/// @private
template <typename T>
struct OpAdd {
  void operator()(T* left, add_cr_non_integral_t<T> right) const { (*left) += right; }
};

/// @brief Minus two value typed by T, and store result in first argument.
/// @private
template <typename T>
struct OpMinus {
  void operator()(T* left, add_cr_non_integral_t<T> right) const { (*left) -= right; }
};

/// @brief Find miner value Typed by T, and store result in first argument.
/// @private
template <class T>
struct OpMin {
  void operator()(T* left, add_cr_non_integral_t<T> right) const {
    if (*left > right) {
      *left = right;
    }
  }
};

/// @brief Find maxer value Typed by T, and store result in first argument.
/// @private
template <class T>
struct OpMax {
  void operator()(T* left, add_cr_non_integral_t<T> right) const {
    if (*left < right) {
      *left = right;
    }
  }
};

/// @brief Store sum value and sample count, for calculating average value.
/// @private
template <class T>
struct AvgBuffer {
  [[nodiscard]] T Average() const { return num > 0 ? (val / num) : 0; }

  T val{T()};
  std::int64_t num{0};
};

/// @brief Dereference parent/global value from thread-local.
/// @private
template <typename TLS>
class GlobalValue {
 public:
  using WriteMostlyType = typename TLS::ParentType;
  using WriteBufferType = typename WriteMostlyType::WriteBuffer;
  using ResultType = typename WriteMostlyType::ResultType;

  explicit GlobalValue(TLS* wrapper) : write_buffer_(&wrapper->buffer_), write_mostly_(wrapper->parent_) {}

  ResultType* Lock() {
    write_buffer_->mutex_.unlock();
    write_mostly_->mutex_.lock();
    return &(write_mostly_->exited_thread_combined_);
  }

  void Unlock() {
    write_mostly_->mutex_.unlock();
    write_buffer_->mutex_.lock();
  }

 private:
  WriteBufferType* write_buffer_;
  WriteMostlyType* write_mostly_;
};

namespace detail {

/// @brief Convert to jason value type.
/// @private
template <class T>
Json::Value ToJsonValue(const T& t) {
  if constexpr (std::is_same_v<T, Json::Value>) {
    return t;
  } else if constexpr (std::is_integral_v<T>) {
    if constexpr (std::is_unsigned_v<T>) {
      return Json::Value(static_cast<Json::UInt64>(t));
    } else {
      return Json::Value(static_cast<Json::Int64>(t));
    }
  } else if constexpr (std::is_floating_point_v<T> ||
                       std::is_same_v<T, std::string_view> || std::is_same_v<T, std::string> ||
                       std::is_same_v<T, const char*> || std::is_same_v<T, bool>) {
    return Json::Value(t);
  } else {
    return Json::Value(t.ToString());
  }
}

/// @brief Specialization for AvgBuffer.
/// @private
template <class T>
Json::Value ToJsonValue(const AvgBuffer<T>& v) {
  return ToJsonValue(v.Average());
}

/// @brief Specialization for vector.
/// @private
template <class T>
Json::Value ToJsonValue(const std::vector<T>& v) {
  Json::Value value;
  for (auto&& element : v) {
    value.append(ToJsonValue(element));
  }
  return value;
}

/// @brief Specialization for unordered_map.
/// @private
template <class T>
Json::Value ToJsonValue(const std::unordered_map<std::string, T>& v) {
  Json::Value value;
  for (auto&& kv : v) {
    value[kv.first] = ToJsonValue(kv.second);
  }
  return value;
}

}  // namespace detail

}  // namespace trpc::tvar
