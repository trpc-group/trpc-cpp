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
// 1. flare
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
// flare is licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "trpc/common/config/trpc_config.h"
#include "trpc/tvar/common/op_util.h"
#include "trpc/tvar/common/sampler.h"
#include "trpc/tvar/common/tvar_group.h"
#include "trpc/util/align.h"
#include "trpc/util/thread/thread_local.h"

namespace trpc::tvar {

// The following source codes are from flare.
// Copied and modified from
// https://github.com/Tencent/flare/blob/master/flare/base/write_mostly/write_mostly.h

/// @brief Using thread local to improve performance in write mostly scenario.
template <class Traits>
class WriteMostly {
  template <typename>
  friend class GlobalValue;

 public:
  using Type = typename Traits::Type;
  using WriteBuffer = typename Traits::WriteBuffer;
  using ResultType = typename Traits::ResultType;
  using InputDataType = typename Traits::InputDataType;
  using SamplerOp = typename Traits::SamplerOp;
  using SamplerInvOp = typename Traits::SamplerInvOp;
  using ReducerSamplerType = ReducerSampler<WriteMostly, ResultType, SamplerOp, SamplerInvOp>;
  using SeriesSamplerType = SeriesSampler<WriteMostly, ResultType, SamplerOp>;

 public:
  explicit WriteMostly(ResultType r = ResultType(), Type t = Type())
      : result_identity_(std::move(r)),
        buffer_identity_(std::move(t)),
        exited_thread_combined_(result_identity_),
        tls_buffer_([this]() { return new WriteBufferWrapper(this); }) {}

  /// @brief Notify sampler collector when destructed.
  virtual ~WriteMostly() {
    if (reducer_sampler_) {
      reducer_sampler_->Destroy();
    }
    if (series_sampler_) {
      series_sampler_->Destroy();
    }
  }

  /// @note Not thread-safe.
  std::weak_ptr<ReducerSamplerType> GetReducerSampler() {
    if (!reducer_sampler_) {
      reducer_sampler_ = std::make_shared<ReducerSamplerType>(this);
      reducer_sampler_->Schedule();
    }
    return reducer_sampler_;
  }

  /// @note Get weak ptr here.
  std::weak_ptr<SeriesSamplerType> GetSeriesSampler() { return series_sampler_; }

  /// @brief Get history data through series sampler.
  std::unordered_map<std::string, std::vector<ResultType>> GetSeriesValue() const noexcept {
    if (series_sampler_) {
      return series_sampler_->GetSeries();
    }
    return std::unordered_map<std::string, std::vector<ResultType>>();
  }

  /// @brief Just write into thread local buffer.
  void Update(InputDataType value) noexcept { Traits::Update(tls_buffer_.Get(), value); }

  /// @brief Merge all thread local buffer into global buffer and reset all.
  ResultType Reset() noexcept {
    Type prev;
    std::unique_lock<std::mutex> lock(mutex_);
    ResultType ret = exited_thread_combined_;
    exited_thread_combined_ = result_identity_;
    tls_buffer_.ForEach([&](auto&& wrapper) {
      wrapper->buffer_.Exchange(&prev, buffer_identity_);
      Traits::Merge(&ret, prev);
    });
    return ret;
  }

  /// @brief Merge all thread local buffer into global buffer without reset.
  ResultType GetValue() const noexcept {
    std::unique_lock<std::mutex> lock(mutex_);
    ResultType ret = exited_thread_combined_;
    tls_buffer_.ForEach([&](auto&& wrapper) { Traits::Merge(&ret, wrapper->buffer_.Load()); });
    return ret;
  }

  /// @brief Thread local buffer.
  /// @private
  struct alignas(hardware_destructive_interference_size) WriteBufferWrapper {
    using ParentType = WriteMostly;

    explicit WriteBufferWrapper(WriteMostly* parent) : parent_(parent) {
      buffer_.Store(parent_->buffer_identity_);
    }

    /// @brief Merged into exited_thread_combined_ and reinit.
    void CommitAndClear() {
      Type prev;
      std::unique_lock<std::mutex> lock(parent_->mutex_);
      buffer_.Exchange(&prev, parent_->buffer_identity_);
      Traits::Merge(&parent_->exited_thread_combined_, prev);
    }

    /// @brief Merged into exited_thread_combined_ when destructed.
    ~WriteBufferWrapper() {
      std::unique_lock<std::mutex> lock(parent_->mutex_);
      Traits::Merge(&parent_->exited_thread_combined_, buffer_.Load());
    }

    // Tvar here.
    WriteMostly* parent_;
    WriteBuffer buffer_;
  };

 protected:
  /// @brief Register to global group.
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

  /// @brief Register to global group start by root.
  std::optional<TrpcVarGroup::Handle> LinkToParent(std::string_view rel_path) {
    return LinkToParent(rel_path, TrpcVarGroup::FindOrCreate("/"));
  }

 private:
  /// @note Not thread-safe.
  bool CheckSeries() {
    if constexpr (is_void_op<SamplerInvOp>::value ||
                  (!std::is_integral_v<ResultType> && !std::is_floating_point_v<ResultType>)) {
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

 private:
  mutable std::mutex mutex_;
  // Used to init exited_thread_combined_.
  ResultType result_identity_;
  // Used to init WriteBufferWrapper::buffer_.
  Type buffer_identity_;
  // Store thread local data of thread exiting.
  mutable ResultType exited_thread_combined_;
  // Core member, thread local wrapper for every threads.
  ThreadLocal<WriteBufferWrapper> tls_buffer_;
  // Used to implement Window/PerSecond.
  std::shared_ptr<ReducerSamplerType> reducer_sampler_;
  // Used to implement history data.
  std::shared_ptr<SeriesSamplerType> series_sampler_;
};

// End of source codes that are from flare.

}  // namespace trpc::tvar
