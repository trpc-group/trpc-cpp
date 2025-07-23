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

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "trpc/common/config/trpc_config.h"
#include "trpc/tvar/basic_ops/reducer.h"
#include "trpc/tvar/common/op_util.h"
#include "trpc/tvar/common/sampler.h"
#include "trpc/tvar/common/series.h"
#include "trpc/tvar/common/tvar_group.h"
#include "trpc/util/log/logging.h"

namespace trpc::tvar {

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/bvar/passive_status.h

/// @brief User passes a function to let framework know how to get current value,
///        so user does not need to update value manually.
template <typename T>
class PassiveStatus {
 public:
  /// The type of function user passed to framework, which recieves no argument and return T value.
  using FucType = std::function<T()>;
  /// Use OpAdd<> to indicate series sampler to sum data up, if series sampler is enabled.
  using SamplerOp = OpAdd<T>;
  /// Use non-void inverse operator to indicate reducer sampler to minus boundary data to
  /// get window value, if reducer sampler is enabled.
  using SamplerInvOp = OpMinus<T>;
  /// Designated reducer sampler type.
  using ReducerSamplerType = ReducerSampler<PassiveStatus, T, SamplerOp, SamplerInvOp>;
  /// Designated series sampler type.
  using SeriesSamplerType = SeriesSampler<PassiveStatus, T, SamplerOp>;

 public:
  /// @brief Not-exposed constructor, tvar is not accessible through admin.
  /// @param fn Function defined by user to get current tvar value.
  explicit PassiveStatus(FucType const& fn) : fn_(fn) {}

  /// @brief Exposed constructor, make tvar accessible through admin.
  /// @param rel_path Relative path to expose tvar with root as parent node, eg, a/b/c.
  /// @param fn User defined function to get current tvar value.
  explicit PassiveStatus(std::string_view rel_path, FucType const& fn) : fn_(fn) {
    handle_ = LinkToParent(rel_path, TrpcVarGroup::FindOrCreate("/"));
  }

  /// @brief Exposed constructor, with group parent set by user, especially when parent is not root.
  /// @param parent Group parent to link to.
  /// @param rel_path Relative path upon parent path.
  /// @param fn User defined funtion to get current value.
  explicit PassiveStatus(TrpcVarGroup* parent, std::string_view rel_path, FucType const& fn)
      : fn_(fn) {
    handle_ = LinkToParent(rel_path, parent);
  }

  /// @note Must release all samplers from sampler collector.
  ~PassiveStatus() {
    if (reducer_sampler_) {
      reducer_sampler_->Destroy();
    }
    if (series_sampler_) {
      series_sampler_->Destroy();
    }
  }

  /// @brief Get reducer sampler, create it if not found.
  /// @note Not thread-safe.
  std::weak_ptr<ReducerSamplerType> GetReducerSampler() {
    if (!reducer_sampler_) {
      reducer_sampler_ = std::make_shared<ReducerSamplerType>(this);
      // Here notify to sampler collector.
      reducer_sampler_->Schedule();
    }
    return reducer_sampler_;
  }

  /// @brief Just call user defined function to get current value and return to user.
  T GetValue() const noexcept { return (fn_ ? fn_() : T()); }

  /// @brief In order to make compiler happy.
  /// @private
  T Reset() {
    TRPC_ASSERT(false && "InternalPassiveStatus::Reset() should never be called, abort");
  }

  /// @brief Get absolute path of current tvar, eg, /a/b/c.
  std::string GetAbsPath() const { return IsExposed() ? handle_->abs_path : std::string(); }

  /// @brief Check current tvar is exposed or not.
  bool IsExposed() const { return handle_.has_value(); }

  /// @brief Just get the series sampler.
  std::weak_ptr<SeriesSamplerType> GetSeriesSampler() { return series_sampler_; }

  /// @brief Get history data upon series sampler, return empty map if not enabled.
  std::unordered_map<std::string, std::vector<T>> GetSeriesValue() const noexcept {
    if (series_sampler_) {
      return series_sampler_->GetSeries();
    }
    return std::unordered_map<std::string, std::vector<T>>();
  }

 private:
  // Check series sampler is enabled or not, if not and allowed, then create it.
  // Not thread-safe.
  bool CheckSeries() {
    // Not all T is allowed to open series sampler.
    if constexpr (!std::is_integral_v<T> && !std::is_floating_point_v<T>) {
      return false;
    }

    // Config is required too.
    if (!trpc::TrpcConfig::GetInstance()->GetGlobalConfig().tvar_config.save_series) {
      return false;
    }

    // Not enable and allowed, just create it.
    if (!series_sampler_) {
      series_sampler_ = std::make_shared<SeriesSamplerType>(this);
      // Notify to sampler collector.
      series_sampler_->Schedule();
    }
    return true;
  }

  // Exposed tvar will try to open series sampler.
  // If series sampler is not enabled, history data is not available in admin.
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
  // User defined function to return current value.
  FucType fn_;
  // To implement window data, to support Window/PerSecond.
  std::shared_ptr<ReducerSamplerType> reducer_sampler_;
  // To implement history data.
  std::shared_ptr<SeriesSamplerType> series_sampler_;
  // Must be last member.
  std::optional<TrpcVarGroup::Handle> handle_;
};

// End of source codes that are from incubator-brpc.

}  // namespace trpc::tvar
