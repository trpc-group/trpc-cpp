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

#include <math.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <utility>

#include "trpc/tvar/common/atomic_type.h"
#include "trpc/tvar/common/fast_rand.h"
#include "trpc/tvar/common/macros.h"
#include "trpc/tvar/common/op_util.h"
#include "trpc/tvar/common/write_mostly.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

namespace trpc::tvar {

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/bvar/detail/percentile.h

/// @brief Expectation of a/b to unsigned.
/// @private
inline uint64_t RoundOfExpectation(uint64_t a, uint64_t b) {
  if (TRPC_UNLIKELY(b == 0)) {
    return 0;
  }
  return a / b + (FastRandLessThan(b) < a % b);
}

/// @brief Store intervals in some time.
/// @private
template <size_t SAMPLE_SIZE>
class PercentileInterval {
 public:
  PercentileInterval() = default;

  /// @brief Get N-th sample in ascending order.
  uint32_t GetSampleAt(size_t index) {
    const size_t saved_num = num_samples_;
    if (index >= saved_num) {
      if (saved_num == 0) {
        return 0;
      }
      index = saved_num - 1;
    }
    if (!sorted_) {
      std::sort(samples_, samples_ + saved_num);
      sorted_ = true;
    }
    TRPC_ASSERT(saved_num == num_samples_);
    return samples_[index];
  }

  /// @brief Try to Merge two PercentileInterval instances.
  template <size_t size2>
  void Merge(PercentileInterval<size2> const& rhs) {
    if (rhs.num_added_ == 0) {
      return;
    }
    static_assert(SAMPLE_SIZE >= size2, "must merge small interval into larger one currently");
    TRPC_ASSERT(rhs.num_samples_ == rhs.num_added_);
    // Fully merged.
    if (num_added_ + rhs.num_added_ <= SAMPLE_SIZE) {
      TRPC_ASSERT(num_samples_ == num_added_);
      memcpy(samples_ + num_samples_, rhs.samples_, sizeof(samples_[0]) * rhs.num_samples_);
      num_samples_ += rhs.num_samples_;
    } else {
      size_t num_remain = RoundOfExpectation(num_added_ * SAMPLE_SIZE, num_added_ + rhs.num_added_);
      TRPC_ASSERT(num_remain <= num_samples_);
      for (size_t i = num_samples_; i > num_remain; --i) {
        samples_[FastRandLessThan(i)] = samples_[i - 1];
      }
      const size_t num_remain_from_rhs = SAMPLE_SIZE - num_remain;
      TRPC_ASSERT(num_remain_from_rhs <= rhs.num_samples_);
      TRPC_DEFINE_SMALL_ARRAY(uint32_t, tmp, rhs.num_samples_, 64);
      memcpy(tmp, rhs.samples_, sizeof(uint32_t) * rhs.num_samples_);
      for (size_t i = 0; i < num_remain_from_rhs; ++i) {
        const int index = FastRandLessThan(rhs.num_samples_ - i);
        samples_[num_remain++] = tmp[index];
        tmp[index] = tmp[rhs.num_samples_ - i - 1];
      }
      num_samples_ = num_remain;
      TRPC_ASSERT(num_samples_ == SAMPLE_SIZE);
    }
    num_added_ += rhs.num_added_;
  }

  /// @brief Select n samples from mutable_rhs into current instance randomly.
  template <size_t size2>
  void MergeWithExpectation(PercentileInterval<size2>& mutable_rhs, size_t n) {
    TRPC_ASSERT(n <= mutable_rhs.num_samples_);
    num_added_ += mutable_rhs.num_added_;
    // Fully merged.
    if (num_samples_ + n <= SAMPLE_SIZE && n == mutable_rhs.num_samples_) {
      memcpy(samples_ + num_samples_, mutable_rhs.samples_, sizeof(samples_[0]) * n);
      num_samples_ += n;
      return;
    }
    // Randomly selected.
    for (size_t i = 0; i < n; ++i) {
      size_t index = FastRandLessThan(mutable_rhs.num_samples_ - i);
      if (num_samples_ < SAMPLE_SIZE) {
        samples_[num_samples_++] = mutable_rhs.samples_[index];
      } else {
        samples_[FastRandLessThan(num_samples_)] = mutable_rhs.samples_[index];
      }
      std::swap(mutable_rhs.samples_[index],
                mutable_rhs.samples_[mutable_rhs.num_samples_ - i - 1]);
    }
  }

  /// @brief Add a latency of uint32_t.
  /// @return True if success.
  bool Add32(uint32_t x) {
    if (TRPC_UNLIKELY(num_samples_ >= SAMPLE_SIZE)) {
      TRPC_FMT_ERROR("This interval was full");
      return false;
    }
    ++num_added_;
    samples_[num_samples_++] = x;
    return true;
  }

  /// @brief Clear all statistics data.
  void Clear() {
    num_added_ = 0;
    sorted_ = false;
    num_samples_ = 0;
  }

  /// @brief Is container full.
  [[nodiscard]] bool Full() const { return num_samples_ == SAMPLE_SIZE; }

  /// @brief Is container empty.
  [[nodiscard]] bool Empty() const { return !num_samples_; }

  /// @brief Ever added by calling add function.
  [[nodiscard]] uint32_t AddedCount() const { return num_added_; }

  /// @brief Get sample count.
  [[nodiscard]] uint32_t SampleCount() const { return num_samples_; }

  /// @brief Return True if two PercentileInterval are exactly same, namely same # of added and
  ///        same samples, mainly for debuggin.
  bool operator==(const PercentileInterval& rhs) const {
    return (num_added_ == rhs.num_added_ && num_samples_ == rhs.num_samples_ &&
            memcmp(samples_, rhs.samples_, num_samples_ * sizeof(uint32_t)) == 0);
  }

 private:
  template <size_t size2>
  friend class PercentileInterval;

  static_assert(SAMPLE_SIZE <= 65536, "SAMPLE_SIZE must be 16bit");
  // How many times add.
  uint32_t num_added_{0};
  // Is samples sorted.
  bool sorted_{false};
  // How many time sampled.
  uint16_t num_samples_{0};
  // Container storing samples.
  uint32_t samples_[SAMPLE_SIZE]{0};
};

/// @private
constexpr size_t NUM_INTERVALS = 32;
struct AddLatency;

/// @brief A group of PercentileIntervals.
/// @private
template <size_t SAMPLE_SIZE_IN>
class PercentileSamples {
 public:
  friend struct AddLatency;

  static const size_t SAMPLE_SIZE = SAMPLE_SIZE_IN;

  PercentileSamples() = default;

  ~PercentileSamples() {
    for (auto&& interval : intervals_) {
      if (interval) {
        delete interval;
      }
    }
  }

  /// @brief Copy-construct from another PercentileSamples.
  ///        Copy/assigning happen at per-second scale. should be OK.
  PercentileSamples(const PercentileSamples& rhs) {
    num_added_ = rhs.num_added_;
    for (size_t i = 0; i < NUM_INTERVALS; ++i) {
      if (rhs.intervals_[i] && !rhs.intervals_[i]->Empty()) {
        intervals_[i] = new PercentileInterval<SAMPLE_SIZE>(*rhs.intervals_[i]);
      } else {
        intervals_[i] = nullptr;
      }
    }
  }

  /// @brief From another PercentileSamples.
  /// @note Keep empty intervals_ to avoid future allocations.
  void operator=(const PercentileSamples& rhs) {
    num_added_ = rhs.num_added_;
    for (size_t i = 0; i < NUM_INTERVALS; ++i) {
      if (rhs.intervals_[i] && !rhs.intervals_[i]->Empty()) {
        GetIntervalAt(i) = *rhs.intervals_[i];
      } else if (intervals_[i]) {
        intervals_[i]->Clear();
      }
    }
  }

  /// @brief Get the `ratio'-ile value. E.g. 0.99 means 99%-ile value.
  ///        Since we store samples in different intervals internally. We first
  ///        address the interval by multiplying ratio with num_added_, then
  ///        find the sample inside the interval. We've tested an alternative
  ///        method that store all samples together w/o any intervals (or in another
  ///        word, only one interval), the method is much simpler but is not as
  ///        stable as current impl. CDF plotted by the method changes dramatically
  ///        from seconds to seconds. It seems that separating intervals probably
  ///        keep more long-tail values.
  uint32_t GetNumber(double ratio) {
    auto n = static_cast<size_t>(ceil(ratio * num_added_));
    if (n > num_added_) {
      n = num_added_;
    } else if (n == 0) {
      return 0;
    }
    for (auto&& interval : intervals_) {
      if (interval == nullptr) {
        continue;
      }
      PercentileInterval<SAMPLE_SIZE>& invl = *interval;
      if (n <= invl.AddedCount()) {
        size_t sample_n = n * invl.SampleCount() / invl.AddedCount();
        size_t sample_index = (sample_n ? sample_n - 1 : 0);
        return invl.GetSampleAt(sample_index);
      }
      n -= invl.AddedCount();
    }
    TRPC_ASSERT(false && "Can't reach here");
    return std::numeric_limits<uint32_t>::max();
  }

  /// @brief Samples in another PercentileSamples.
  template <size_t size2>
  void Merge(const PercentileSamples<size2>& rhs) {
    num_added_ += rhs.num_added_;
    for (size_t i = 0; i < NUM_INTERVALS; ++i) {
      if (rhs.intervals_[i] && !rhs.intervals_[i]->Empty()) {
        GetIntervalAt(i).Merge(*rhs.intervals_[i]);
      }
    }
  }

  /// @brief Multiple into a single PercentileSamples.
  template <typename Iterator>
  void CombineOf(const Iterator& begin, const Iterator& end) {
    if (num_added_) {
      // Very unlikely.
      for (auto&& interval : intervals_) {
        if (interval) {
          interval->Clear();
        }
      }
      num_added_ = 0;
    }

    for (Iterator iter = begin; iter != end; ++iter) {
      num_added_ += iter->num_added_;
    }

    // Calculate probabilities for each interval.
    for (size_t i = 0; i < NUM_INTERVALS; ++i) {
      size_t total = 0;
      size_t total_sample = 0;
      for (Iterator iter = begin; iter != end; ++iter) {
        if (iter->intervals_[i]) {
          total += iter->intervals_[i]->AddedCount();
          total_sample += iter->intervals_[i]->SampleCount();
        }
      }
      if (total == 0) {
        // Empty interval.
        continue;
      }

      // Consider that sub interval took |a| samples out of |b| totally,
      // each sample won the probability of |a/b| according to the
      // algorithm of add32(), now we should pick some samples into the
      // combined interval that satisfied each sample has the
      // probability of |SAMPLE_SIZE/total|, so each sample has the
      // probability of |(SAMPLE_SIZE*b)/(a*total) to remain and the
      // expected number of samples in this interval is
      // |(SAMPLE_SIZE*b)/total|
      for (Iterator iter = begin; iter != end; ++iter) {
        if (!iter->intervals_[i] || iter->intervals_[i]->Empty()) {
          continue;
        }
        typename std::add_lvalue_reference_t<decltype(*(iter->intervals_[i]))> invl =
            *(iter->intervals_[i]);
        if (total <= SAMPLE_SIZE) {
          GetIntervalAt(i).MergeWithExpectation(invl, invl.SampleCount());
          continue;
        }
        // Each
        const size_t b = invl.AddedCount();
        const size_t remain =
            std::min(RoundOfExpectation(b * SAMPLE_SIZE, total), (size_t)invl.SampleCount());
        GetIntervalAt(i).MergeWithExpectation(invl, remain);
      }
    }
  }

  /// @brief If intervals of two PercentileSamples are exactly same.
  bool operator==(const PercentileSamples& rhs) const {
    for (size_t i = 0; i < NUM_INTERVALS; ++i) {
      if (intervals_ != rhs.intervals_[i]) {
        return false;
      }
    }
    return true;
  }

  /// @brief For test.
  auto GetIntervals() { return intervals_; }

 private:
  template <size_t size1>
  friend class PercentileSamples;

  /// @brief Get/create interval on-demand.
  PercentileInterval<SAMPLE_SIZE>& GetIntervalAt(size_t index) {
    if (intervals_[index] == nullptr) {
      intervals_[index] = new PercentileInterval<SAMPLE_SIZE>;
    }
    return *intervals_[index];
  }
  // Sum of num_added_ of all intervals. we update this value after any
  // changes to intervals inside to make it O(1)-time accessible.
  size_t num_added_{0};
  PercentileInterval<SAMPLE_SIZE>* intervals_[NUM_INTERVALS]{nullptr};
};

template <size_t sz>
const size_t PercentileSamples<sz>::SAMPLE_SIZE;

/// @note we intentionally minus 2 uint32_t from sample-size to make the struct
///       size be power of 2 and more friendly to memory allocators.
/// @private
using GlobalPercentileSamples = PercentileSamples<254>;
/// @private
using ThreadLocalPercentileSamples = PercentileSamples<30>;

/// @brief A specialized reducer for finding the percentile of latencies.
/// @note DON'T use it directly, use LatencyRecorder instead.
/// @private
class PercentileTraits {
 public:
  struct AddPercentileSamples {
    template <size_t size1, size_t size2>
    void operator()(PercentileSamples<size1>* b1, PercentileSamples<size2> const& b2) const {
      b1->Merge(b2);
    }
  };

 public:
  using Type = ThreadLocalPercentileSamples;
  using WriteBuffer = AtomicType<Type>;
  using InputDataType = std::uint32_t;
  using ResultType = GlobalPercentileSamples;
  using SamplerOp = AddPercentileSamples;
  using SamplerInvOp = VoidOp;

 public:
  PercentileTraits(PercentileTraits const&) = delete;

  void operator=(PercentileTraits const&) = delete;

  PercentileTraits() = default;

  /// @brief Updata thread local data.
  template <typename TLS>
  static void Update(TLS* wb, InputDataType val) {
    GlobalValue<TLS> g(wb);
    wb->buffer_.template MergeGlobal<AddLatency, GlobalValue<TLS>, InputDataType>(&g, val);
  }

  /// @brief Merge thread local data into global.
  static void Merge(ResultType* wb1, Type const& wb2) { AddPercentileSamples()(wb1, wb2); }
};

/// @brief Operator to add latancy.
/// @private
struct AddLatency {
  void operator()(GlobalValue<WriteMostly<PercentileTraits>::WriteBufferWrapper>* global_value,
                  ThreadLocalPercentileSamples* local_value, uint32_t latency);
};

/// @brief High performance latency counter.
/// @private
class WriteMostlyPercentile : public WriteMostly<PercentileTraits> {
 public:
  WriteMostlyPercentile()
      : WriteMostly<PercentileTraits>(PercentileTraits::ResultType{}, PercentileTraits::Type{}) {}
};

// End of source codes that are from incubator-brpc.

}  // namespace trpc::tvar
