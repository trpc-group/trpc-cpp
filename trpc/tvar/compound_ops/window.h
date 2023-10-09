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

#include <ctime>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "trpc/common/config/trpc_config.h"
#include "trpc/tvar/common/sampler.h"
#include "trpc/tvar/common/series.h"
#include "trpc/tvar/common/tvar_group.h"
#include "trpc/util/log/logging.h"

namespace trpc::tvar {

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/bvar/window.h

/// @private
enum class SeriesFrequency {
  // Smooth in last window.
  SERIES_IN_WINDOW = 0,
  // Smooth in last second.
  SERIES_IN_SECOND = 1
};

/// @note R must has member GetReducerSampler() and define ReducerSamplerType.
/// @private
template <typename R, SeriesFrequency series_freq>
class WindowBase {
 public:
  using ReducerSamplerType = typename R::ReducerSamplerType;
  using OpType = typename ReducerSamplerType::OpType;
  using DataType = typename ReducerSamplerType::DataType;

  class SeriesSamplerType : public Sampler {
   public:
    explicit SeriesSamplerType(WindowBase* owner) : owner_(owner), series_() {}

    ~SeriesSamplerType() override = default;

    void TakeSample() override {
      if (series_freq == SeriesFrequency::SERIES_IN_SECOND) {
        // Get one-second window value for PerSecond<>, otherwise the
        // smoother plot may hide peaks.
        series_.Append(owner_->GetValue(1));
      } else {
        // Get the value inside the full window. get_value(1) is
        // incorrect when users intend to see aggregated values of
        // the full window in the plot.
        series_.Append(owner_->GetValue());
      }
    }

    std::unordered_map<std::string, std::vector<DataType>> GetSeries() const noexcept {
      return series_.GetSeries();
    }

   private:
    WindowBase* owner_;
    Series<DataType, OpType> series_;
  };

  explicit WindowBase(R* var, time_t window_size = -1)
      : var_(var),
        window_size_(
            window_size > 0
                ? window_size
                : trpc::TrpcConfig::GetInstance()->GetGlobalConfig().tvar_config.window_size),
        reducer_sampler_(var->GetReducerSampler()) {
    TRPC_ASSERT(reducer_sampler_.lock()->SetWindowSize(window_size_) == 0);
  }

  /// @brief Only need to destroy own series sampler.
  virtual ~WindowBase() {
    if (series_sampler_) {
      series_sampler_->Destroy();
    }
  }

  bool GetSpan(time_t window_size, Sample<DataType>* result) const {
    if (auto ptr = reducer_sampler_.lock()) {
      return ptr->GetValue(window_size, result);
    }
    return false;
  }

  bool GetSpan(Sample<DataType>* result) const { return GetSpan(window_size_, result); }

  virtual DataType GetValue(time_t window_size) const noexcept {
    Sample<DataType> tmp;
    if (GetSpan(window_size, &tmp)) {
      return tmp.data;
    }
    return DataType();
  }

  virtual DataType GetValue() const noexcept { return GetValue(window_size_); }

  [[nodiscard]] time_t WindowSize() const { return window_size_; }

  void GetSamples(std::vector<DataType>* samples) const {
    if (auto ptr = reducer_sampler_.lock()) {
      samples->clear();
      samples->reserve(window_size_);
      return ptr->GetSamples(samples, window_size_);
    }
  }

  std::weak_ptr<SeriesSamplerType> GetSeries() { return series_sampler_; }

  std::unordered_map<std::string, std::vector<DataType>> GetSeriesValue() const noexcept {
    if (series_sampler_) {
      return series_sampler_->GetSeries();
    }
    return std::unordered_map<std::string, std::vector<DataType>>();
  }

 protected:
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

  std::optional<TrpcVarGroup::Handle> LinkToParent(std::string_view rel_path) {
    return LinkToParent(rel_path, TrpcVarGroup::FindOrCreate("/"));
  }

private:
  /// @brief Not thread-safe.
  bool CheckSeries() {
    if (!trpc::TrpcConfig::GetInstance()->GetGlobalConfig().tvar_config.save_series) {
      return false;
    }

    if (!series_sampler_) {
      series_sampler_ = std::make_shared<SeriesSamplerType>(this);
      series_sampler_->Schedule();
    }
    return true;
  }

 protected:
  R* var_;
  time_t window_size_;
  // Must be last.
  std::weak_ptr<ReducerSamplerType> reducer_sampler_;
  std::shared_ptr<SeriesSamplerType> series_sampler_;
};

/// @brief To retrieve statistics in a window time.
template <typename R, SeriesFrequency series_freq = SeriesFrequency::SERIES_IN_WINDOW>
class Window : public WindowBase<R, series_freq> {
 private:
  using Base = WindowBase<R, series_freq>;

 public:
  explicit Window(R* var, time_t window_size = -1) : Base(var, window_size) {}

  Window(std::string_view rel_path, R* var, time_t window_size = -1) : Window(var, window_size) {
    handle_ = Base::LinkToParent(rel_path);
  }

  Window(TrpcVarGroup* parent, std::string_view rel_path, R* var, time_t window_size = -1)
      : Window(var, window_size) {
    handle_ = Base::LinkToParent(rel_path, parent);
  }

  std::string GetAbsPath() const { return IsExposed() ? handle_->abs_path : std::string(); }

  bool IsExposed() const { return handle_.has_value(); }

 private:
  std::optional<TrpcVarGroup::Handle> handle_;
};

/// @brief To retrieve per second statistics in a window.
template <typename R, SeriesFrequency series_freq = SeriesFrequency::SERIES_IN_SECOND>
class PerSecond : public WindowBase<R, series_freq> {
 private:
  using Base = WindowBase<R, series_freq>;
  using ReducerSamplerType = typename R::ReducerSamplerType;
  using DataType = typename ReducerSamplerType::DataType;

 public:
  explicit PerSecond(R* var, time_t window_size = -1) : Base(var, window_size) {}

  PerSecond(std::string_view rel_path, R* var, time_t window_size = -1)
      : PerSecond(var, window_size) {
    handle_ = Base::LinkToParent(rel_path);
  }

  PerSecond(TrpcVarGroup* parent, std::string_view rel_path, R* var, time_t window_size = -1)
      : PerSecond(var, window_size) {
    handle_ = Base::LinkToParent(rel_path, parent);
  }

  DataType GetValue(time_t window_size) const noexcept override {
    Sample<DataType> s;
    if (auto ptr = this->reducer_sampler_.lock()) {
      ptr->GetValue(window_size, &s);
    }
    if (s.time_us <= 0) {
      return static_cast<DataType>(0);
    }
    if (std::is_floating_point_v<DataType>) {
      return static_cast<DataType>(s.data * 1000000.0 / s.time_us);
    } else {
      return static_cast<DataType>(round(s.data * 1000000.0 / s.time_us));
    }
  }

  DataType GetValue() const noexcept override { return Base::GetValue(); }

  std::string GetAbsPath() const { return IsExposed() ? handle_->abs_path : std::string(); }

  bool IsExposed() const { return handle_.has_value(); }

 private:
  std::optional<TrpcVarGroup::Handle> handle_;
};

// End of source codes that are from incubator-brpc.

}  // namespace trpc::tvar
