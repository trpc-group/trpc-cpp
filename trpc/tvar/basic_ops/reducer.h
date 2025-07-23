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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. incubator-brpc
// Copyright (C) 2019 The Apache Software Foundation
// incubator-brpc is licensed under the Apache 2.0 License.
//
//

#pragma once

#include <limits>
#include <optional>
#include <string>
#include <string_view>

#include "trpc/tvar/basic_ops/basic_traits.h"
#include "trpc/tvar/common/op_util.h"
#include "trpc/tvar/common/tvar_group.h"
#include "trpc/tvar/common/write_mostly.h"
#include "trpc/util/log/logging.h"

namespace trpc::tvar {

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/bvar/reducer.h

/// @private
template <typename T, typename Op, typename InvOp = VoidOp>
using CumulativeTraits = BasicTraits<T, add_cr_non_integral_t<T>, T, Op, Op, Op, InvOp>;

/// @brief For Counter and Gauge categories.
/// @private
template <class T>
using AddTraits = CumulativeTraits<T, OpAdd<T>, OpMinus<T>>;

/// @brief For Miner category.
/// @private
template <class T>
using MinTraits = CumulativeTraits<T, OpMin<T>>;

/// @brief For Maxer category.
/// @private
template <class T>
using MaxTraits = CumulativeTraits<T, OpMax<T>>;

/// @brief Only increase.
template <typename T, typename Base = WriteMostly<AddTraits<T>>>
class Counter : public Base {
 public:
  /// @brief Not-exposed constructor, not available through admin.
  explicit Counter(add_cr_non_integral_t<T> init_value = T()) : Base(init_value, init_value) {}

  /// @brief Exposed constructor upon root.
  explicit Counter(std::string_view rel_path, add_cr_non_integral_t<T> init_value = T())
      : Counter(init_value) {
    handle_ = Base::LinkToParent(rel_path);
  }

  /// @brief Exposed constructor upon user set parent.
  Counter(TrpcVarGroup* parent, std::string_view rel_path,
          add_cr_non_integral_t<T> init_value = T())
      : Counter(init_value) {
    handle_ = Base::LinkToParent(rel_path, parent);
  }

  /// @brief Not allowed to decrease.
  void Add(add_cr_non_integral_t<T> value) noexcept {
    TRPC_ASSERT(value >= 0);
    Base::Update(value);
  }

  /// @brief Add by 1.
  void Increment() noexcept { Add(1); }

  /// @brief Get absolute path.
  std::string GetAbsPath() const { return IsExposed() ? handle_->abs_path : std::string(); }

  /// @brief Check exposed or not.
  bool IsExposed() const { return handle_.has_value(); }

 private:
  // Keep this member last to avoid race condition.
  std::optional<TrpcVarGroup::Handle> handle_;
};

/// @brief Both increase and decrease is permitted.
template <typename T, typename Base = WriteMostly<AddTraits<T>>>
class Gauge : public Base {
 public:
  /// @brief Not-exposed, and not available through admin.
  /// @param init_value User set init value.
  explicit Gauge(add_cr_non_integral_t<T> init_value = T()) : Base(init_value, init_value) {}

  /// @brief Exposed upon root.
  explicit Gauge(std::string_view rel_path, add_cr_non_integral_t<T> init_value = T())
      : Gauge(init_value) {
    handle_ = Base::LinkToParent(rel_path);
  }

  /// @brief Exposed upon user set parent.
  Gauge(TrpcVarGroup* parent, std::string_view rel_path, add_cr_non_integral_t<T> init_value = T())
      : Gauge(init_value) {
    handle_ = Base::LinkToParent(rel_path, parent);
  }

  /// @note User input value must bigger than or equal to 0.
  void Add(add_cr_non_integral_t<T> value) noexcept {
    TRPC_ASSERT(value >= 0);
    Base::Update(value);
  }

  /// @brief Allowed to decrease.
  void Subtract(add_cr_non_integral_t<T> value) noexcept {
    TRPC_ASSERT(value >= 0);
    Base::Update(-value);
  }

  /// @brief Add by 1.
  void Increment() noexcept { Add(1); }

  /// @brief Subtract by 1.
  void Decrement() noexcept { Subtract(1); }

  /// @brief Get absolute path.
  std::string GetAbsPath() const { return IsExposed() ? handle_->abs_path : std::string(); }

  /// @brief Check exposed.
  bool IsExposed() const { return handle_.has_value(); }

 private:
  // Last one.
  std::optional<TrpcVarGroup::Handle> handle_;
};

/// @brief Record miner.
template <typename T, typename Base = WriteMostly<MinTraits<T>>>
class Miner : public Base {
 public:
  /// @brief Not exposed to admin.
  explicit Miner(add_cr_non_integral_t<T> init_value = std::numeric_limits<T>::max())
      : Base(init_value, init_value) {}

  /// @brief Exposed upon root.
  explicit Miner(std::string_view rel_path,
                 add_cr_non_integral_t<T> init_value = std::numeric_limits<T>::max())
      : Miner(init_value) {
    handle_ = Base::LinkToParent(rel_path);
  }

  /// @brief Exposed upon user set parent.
  Miner(TrpcVarGroup* parent, std::string_view rel_path,
        add_cr_non_integral_t<T> init_value = std::numeric_limits<T>::max())
      : Miner(init_value) {
    handle_ = Base::LinkToParent(rel_path, parent);
  }

  /// @brief Get absolute path, empty string is returned if not exposed.
  std::string GetAbsPath() const { return IsExposed() ? handle_->abs_path : std::string(); }

  /// @brief Check exposed or not.
  bool IsExposed() const { return handle_.has_value(); }

 private:
  // Last one.
  std::optional<TrpcVarGroup::Handle> handle_;
};

/// @brief Record maxer.
template <typename T, typename Base = WriteMostly<MaxTraits<T>>>
class Maxer : public Base {
 public:
  /// @note This construcotr will not exposed tvar to admin.
  explicit Maxer(add_cr_non_integral_t<T> init_value = std::numeric_limits<T>::min())
      : Base(init_value, init_value) {}

  /// @brief Exposed upon root node.
  explicit Maxer(std::string_view rel_path,
                 add_cr_non_integral_t<T> init_value = std::numeric_limits<T>::min())
      : Maxer(init_value) {
    handle_ = Base::LinkToParent(rel_path);
  }

  /// @brief Exposed upon user set parent node.
  Maxer(TrpcVarGroup* parent, std::string_view rel_path,
        add_cr_non_integral_t<T> init_value = std::numeric_limits<T>::min())
      : Maxer(init_value) {
    handle_ = Base::LinkToParent(rel_path, parent);
  }

  /// @brief Get absolute path.
  std::string GetAbsPath() const { return IsExposed() ? handle_->abs_path : std::string(); }

  /// @brief Tvar is exposed nor not.
  bool IsExposed() const { return handle_.has_value(); }

 private:
  // Has to be last to avoid race condition.
  std::optional<TrpcVarGroup::Handle> handle_;
};

// End of source codes that are from incubator-brpc.

}  // namespace trpc::tvar
