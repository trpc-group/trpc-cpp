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

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "trpc/common/config/trpc_config.h"
#include "trpc/tvar/common/atomic_type.h"
#include "trpc/tvar/common/op_util.h"
#include "trpc/tvar/common/sampler.h"
#include "trpc/tvar/common/series.h"
#include "trpc/tvar/common/tvar_group.h"

/// @brief Namespace to wrap tvar interfaces.
namespace trpc::tvar {

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/bvar/status.h

/// @brief User needs to update value manually.
template <typename T>
class Status {
 public:
  /// Make Get/Set thread-safe.
  using WriteBuffer = AtomicType<T>;
  /// Designated series sampler type.
  using SeriesSamplerType = SeriesSampler<Status, T, OpAdd<T>>;

 public:
  /// @brief Reserved constructor, not-exposed.
  Status() = default;

  /// @brief Not-exposed constructor with value set.
  explicit Status(T const& value) { value_.Store(value); }

  /// @brief Exposed constructor with root as parent node.
  /// @param rel_path Relative path upon root.
  /// @param value Init value.
  explicit Status(std::string_view rel_path, T const& value) : Status(value) {
    handle_ = LinkToParent(rel_path, TrpcVarGroup::FindOrCreate("/"));
  }

  /// @brief Exposed constructor with parent node set by user.
  /// @param parent Parent node based on.
  /// @param rel_path Relative path upon parent node.
  /// @param value Init value.
  explicit Status(TrpcVarGroup* parent, std::string_view rel_path, T const& value) : Status(value) {
    handle_ = LinkToParent(rel_path, parent);
  }

  /// @brief Notify sampler collector if series sampler enabled.
  ~Status() {
    if (series_sampler_) {
      series_sampler_->Destroy();
    }
  }

  /// @brief User call this function to update Status value.
  /// @note Thread-safe.
  void Update(T const& value) noexcept { value_.Store(value); }

  /// @brief Same as Update.
  void SetValue(T const& value) noexcept { return Update(value); }

  /// @brief Get Status value.
  /// @note Thread-safe.
  T GetValue() const noexcept { return value_.Load(); }

  /// @brief Get absolute path, empty string returned if not exposed.
  std::string GetAbsPath() const { return IsExposed() ? handle_->abs_path : std::string(); }

  /// @brief Check tvar is exposed or not.
  bool IsExposed() const { return handle_.has_value(); }

  /// @brief Get series sampler.
  /// @note Weak ptr is returned.
  std::weak_ptr<SeriesSamplerType> GetSeriesSampler() { return series_sampler_; }

  /// @brief Get tvar hsitory data.
  /// @note Empty map is returned if series sampler is not enabled.
  std::unordered_map<std::string, std::vector<T>> GetSeriesValue() const noexcept {
    if (series_sampler_) {
      return series_sampler_->GetSeries();
    }
    return std::unordered_map<std::string, std::vector<T>>();
  }

 private:
  /// @note Not thread-safe.
  bool CheckSeries() {
    if constexpr (!std::is_integral_v<T> && !std::is_floating_point_v<T>) {
      return false;
    }

    if (!trpc::TrpcConfig::GetInstance()->GetGlobalConfig().tvar_config.save_series) {
      return false;
    }

    if (!series_sampler_) {
      series_sampler_ = std::make_shared<SeriesSamplerType>(this);
      series_sampler_->Schedule();
    }
    return true;
  }

  /// @brief Try to open series sampler if available.
  std::optional<TrpcVarGroup::Handle> LinkToParent(std::string_view rel_path,
                                                   TrpcVarGroup* parent) {
    if (CheckSeries()) {
      return TrpcVarGroup::LinkToParent(
          rel_path, parent, [this] { return detail::ToJsonValue(GetValue()); },
          [this] { return detail::ToJsonValue(GetSeriesValue()); });
    } else {
      return TrpcVarGroup::LinkToParent(rel_path, parent,
                                        [this] { return detail::ToJsonValue(GetValue()); });
    }
  }

 private:
  // Protected by AtomicType.
  WriteBuffer value_;
  // To implement history data.
  std::shared_ptr<SeriesSamplerType> series_sampler_;
  // Must be last member.
  std::optional<TrpcVarGroup::Handle> handle_;
};

// End of source codes that are from incubator-brpc.

}  // namespace trpc::tvar
