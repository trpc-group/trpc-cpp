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

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "trpc/tvar/common/op_util.h"
#include "trpc/tvar/common/series.h"
#include "trpc/util/container/bounded_queue.h"
#include "trpc/util/log/logging.h"

namespace trpc::tvar {

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/bvar/detail/sampler.h

/// @brief Associate value with timestamp.
/// @private
template <typename T>
struct Sample {
  Sample() = default;

  Sample(T const& d, uint64_t t) : data(d), time_us(t) {}

  T data;

  uint64_t time_us{0};
};

/// @brief Shared ptr wiil be hold by sampler_collector thread after Schedule called.
///        And sampler_collector thread will call member function TakeSample periodically.
/// @private
class Sampler : public std::enable_shared_from_this<Sampler> {
 public:
  /// @brief Default constructor is discard.
  Sampler();

  virtual void TakeSample() = 0;

  /// @brief Register shared ptr to sampler_collector thread.
  void Schedule();

  /// @brief Tell sampler_collector to release shared ptr.
  void Destroy();

 protected:
  virtual ~Sampler() = default;

  friend class SamplerCollector;

  // Let holding thread know when to release shared ptr.
  bool used_;
  // To protect used_.
  mutable std::mutex mutex_;
};

/// @brief To Store history data, R must have GetValue() method.
template <typename R, typename T, typename Op>
class SeriesSampler : public Sampler {
 public:
  using SeriesOp =
        std::conditional_t<std::is_integral_v<T> || std::is_floating_point_v<T>, Op, VoidOp>;

  explicit SeriesSampler(R* owner) : owner_(owner), series_() {}

  ~SeriesSampler() override = default;

  void TakeSample() override { series_.Append(owner_->GetValue()); }

  std::unordered_map<std::string, std::vector<T>> GetSeries() const noexcept {
    return series_.GetSeries();
  }

 private:
  R* owner_;
  Series<T, SeriesOp> series_;
};

/// @brief Template parameter R must have below functions, or compile will failed.
///        1. T Reset();
///        2. T GetValue();
template <typename R, typename T, typename Op, typename InvOp>
class ReducerSampler : public Sampler {
 public:
  using DataType = T;
  using OpType = Op;
  static const time_t MAX_SECONDS_LIMIT = 3600;

  /// @brief Reducer must provided.
  explicit ReducerSampler(R* reducer) : reducer_(reducer), window_size_(1) {
    // Take first sample when initailized.
    this->TakeSample();
  }

  ~ReducerSampler() {}

  /// @brief Periodically invoked by sampler_collector thread, if scheduled.
  void TakeSample() override {
    // window_size may raise.
    if (static_cast<size_t>(window_size_) + 1 > queue_.Capacity()) {
      const size_t new_cap = std::max(queue_.Capacity() * 2, (size_t)window_size_ + 1);
      const size_t mem_size = sizeof(Sample<T>) * new_cap;
      void* mem = malloc(mem_size);
      if (mem == nullptr) {
        return;
      }
      trpc::container::BoundedQueue<Sample<T> > new_q(mem, mem_size,
                                                      trpc::container::StorageOwnership::OWNS_STORAGE);
      Sample<T> tmp;
      while (queue_.Pop(&tmp)) {
        new_q.Push(tmp);
      }
      new_q.Swap(queue_);
    }

    Sample<T> latest;
    if (is_void_op<InvOp>::value) {
      latest.data = reducer_->Reset();
    } else {
      latest.data = reducer_->GetValue();
    }

    latest.time_us = GetSystemMicroseconds();
    queue_.ElimPush(latest);
  }

  /// @brief Get windowed value inside latest window_size.
  bool GetValue(time_t window_size, Sample<T>* result) const {
    if (window_size <= 0) {
      TRPC_FMT_ERROR("Invalid window_size={}", window_size);
      return false;
    }

    auto&& const_queue = std::as_const(queue_);

    std::unique_lock<std::mutex> lock(mutex_);
    if (const_queue.Size() <= 1UL) {
      return false;
    }
    auto* oldest = const_queue.Bottom(window_size);
    if (oldest == nullptr) {
      oldest = const_queue.Top();
    }
    auto* latest = const_queue.Bottom();
    TRPC_ASSERT(latest != oldest);
    if (is_void_op<InvOp>::value) {
      result->data = latest->data;
      for (int i = 1; true; ++i) {
        auto* e = const_queue.Bottom(i);
        if (e == oldest) {
          break;
        }
        Op()(&result->data, e->data);
      }
    } else {
      result->data = latest->data;
      InvOp()(&result->data, oldest->data);
    }
    result->time_us = latest->time_us - oldest->time_us;
    return true;
  }

  /// @brief Set new window size, thread-safe.
  int SetWindowSize(time_t window_size) {
    if (window_size <= 0 || window_size > MAX_SECONDS_LIMIT) {
      TRPC_FMT_ERROR("Invalid window_size={}", window_size);
      return -1;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    // window_size is not allowed to reduce.
    if (window_size > window_size_) {
      window_size_ = window_size;
    }

    return 0;
  }

  /// @brief Get current window size, thread-safe.
  time_t GetWindowSize() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return window_size_;
  }

  /// @brief Get all sample data except timestamp inside last window_size.
  void GetSamples(std::vector<T>* samples, time_t window_size) const {
    if (window_size <= 0) {
      TRPC_FMT_ERROR("Invalid window_size={}", window_size);
      return;
    }

    auto&& const_queue = std::as_const(queue_);

    std::unique_lock<std::mutex> lock(mutex_);
    if (const_queue.Size() <= 1) {
      return;
    }

    auto* oldest = const_queue.Bottom(window_size);
    if (oldest == nullptr) {
      oldest = const_queue.Top();
    }
    for (int i = 1; true; ++i) {
      auto* e = const_queue.Bottom(i);
      if (e == oldest) {
        break;
      }
      samples->emplace_back(e->data);
    }
  }

 private:
  // Parent containing current reducer sampler.
  R* reducer_;
  // In order to limit container size.
  time_t window_size_;
  // Container holding datas.
  trpc::container::BoundedQueue<Sample<T> > queue_;
};

// End of source codes that are from incubator-brpc.

/// @brief To start sampler_collector thread.
/// @private
void SamplerCollectorThreadStart();

/// @brief To stop sampler_collector thread.
/// @private
void SamplerCollectorThreadStop();

}  // namespace trpc::tvar
