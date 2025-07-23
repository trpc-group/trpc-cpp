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

#include <cstdint>
#include <limits>
#include <optional>
#include <string>

#include "trpc/tvar/basic_ops/basic_traits.h"
#include "trpc/tvar/common/atomic_type.h"
#include "trpc/tvar/common/op_util.h"
#include "trpc/tvar/common/tvar_group.h"
#include "trpc/tvar/common/write_mostly.h"
#include "trpc/util/likely.h"
#include "trpc/util/log/logging.h"

namespace trpc::tvar {

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/bvar/recorder.h

/// @brief Insert one value into avg buffer.
/// @private
template <class T>
struct UpdateAvgBuffer {
  void operator()(AvgBuffer<T>* left, add_cr_non_integral_t<T> right) const {
    left->val += right;
    ++left->num;
  }
};

/// @brief Merge two avg buffer.
/// @private
template <class T>
struct MergeAvgBuffer {
  void operator()(AvgBuffer<T>* left, add_cr_non_integral_t<AvgBuffer<T>> right) const {
    left->val += right.val;
    left->num += right.num;
  }
};

/// @brief Minus two avg buffer.
/// @private
template <class T>
struct MinusAvgBuffer {
  void operator()(AvgBuffer<T>* left, add_cr_non_integral_t<AvgBuffer<T>> right) const {
    left->val -= right.val;
    left->num -= right.num;
  }
};

/// @private
template <class T>
using AvgTraits = BasicTraits<AvgBuffer<T>, add_cr_non_integral_t<T>, AvgBuffer<T>, UpdateAvgBuffer<T>,
                  MergeAvgBuffer<T>, MergeAvgBuffer<T>, MinusAvgBuffer<T>>;

/// @brief High Performance average trait for only int value.
/// @private
struct CompressedIntAvgTraits {
  using Type = std::int64_t;
  using WriteBuffer = AtomicType<Type>;
  using InputDataType = std::int64_t;
  using ResultType = AvgBuffer<Type>;
  using SamplerOp = MergeAvgBuffer<Type>;
  using SamplerInvOp = MinusAvgBuffer<Type>;

  struct CompressedIntAvg {
    // Compressing format:
    // | 20 bits (unsigned) | sign bit | 43 bits |
    //       num                   sum
    static const size_t SUM_BIT_WIDTH = 44;
    static const uint64_t MAX_SUM_PER_THREAD = (1ul << SUM_BIT_WIDTH) - 1;
    static const uint64_t MAX_NUM_PER_THREAD = (1ul << (64ul - SUM_BIT_WIDTH)) - 1;
    static_assert(SUM_BIT_WIDTH > 32, "SUM_BIT_WIDTH_must_be_between_33_and_63");
    static_assert(SUM_BIT_WIDTH < 64, "SUM_BIT_WIDTH_must_be_between_33_and_63");

    static uint64_t GetSum(int64_t n) { return (static_cast<uint64_t>(n) & MAX_SUM_PER_THREAD); }

    static uint64_t GetNum(int64_t n) { return static_cast<uint64_t>(n) >> SUM_BIT_WIDTH; }

    static uint64_t Compress(uint64_t num, uint64_t sum) {
      return (num << SUM_BIT_WIDTH)
             // There is a redundant '1' in the front of sum which was
             // combined with two negative number, so truncation has to be
             // done here
             | (sum & MAX_SUM_PER_THREAD);
    }

    static bool WillOverflow(int64_t lhs, int rhs) {
      return
          // Both integers are positive and the sum is larger than the largest
          // number
          ((lhs > 0) && (rhs > 0) && (lhs + rhs > ((int64_t)MAX_SUM_PER_THREAD >> 1)))
          // Or both integers are negative and the sum is less than the lowest
          // number
          || ((lhs < 0) && (rhs < 0) && (lhs + rhs < (-((int64_t)MAX_SUM_PER_THREAD >> 1)) - 1));
      // otherwise the sum cannot overflow iff lhs does not overflow
      // because |sum| < |lhs|
    }

    /// @brief Fill all the first (64 - SUM_BIT_WIDTH + 1) bits with 1 if the sign bit is 1
    ///        to represent a complete 64-bit negative number
    ///        Check out http://en.wikipedia.org/wiki/Signed_number_representations if you are confused.
    static int64_t ExtendSignBit(uint64_t sum) {
      return (((1ul << (64ul - SUM_BIT_WIDTH + 1)) - 1) * ((1ul << (SUM_BIT_WIDTH - 1) & sum))) |
             (int64_t)sum;
    }

    /// @brief Convert complement into a |SUM_BIT_WIDTH|-bit unsigned integer.
    static uint64_t GetComplement(int64_t n) { return n & (MAX_SUM_PER_THREAD); }
  };

  struct MergeIntBuffer {
    void operator()(ResultType* left, add_cr_non_integral_t<Type> right) {
      left->val +=
          CompressedIntAvg::ExtendSignBit(CompressedIntAvg::GetSum(static_cast<uint64_t>(right)));
      left->num += CompressedIntAvg::GetNum(static_cast<uint64_t>(right));
    }
  };

  /// @brief Update value into thread local.
  template <typename TLS>
  static void Update(TLS* wb, InputDataType val) {
    if (TRPC_UNLIKELY(static_cast<int64_t>(static_cast<int>(val)) != val)) {
      char const* reason = nullptr;
      if (val > std::numeric_limits<int>::max()) {
        reason = "over_flows";
        val = std::numeric_limits<int>::max();
      } else {
        reason = "under_flows";
        val = std::numeric_limits<int>::min();
      }
      TRPC_FMT_WARN("Input={} to IntRecorderTraits {}", val, reason);
    }

    int64_t n = wb->buffer_.Load();
    uint64_t const complement = CompressedIntAvg::GetComplement(val);
    uint64_t num = 0;
    uint64_t sum = 0;
    do {
      num = CompressedIntAvg::GetNum(n);
      sum = CompressedIntAvg::GetSum(n);
      if (TRPC_UNLIKELY(
              (num + 1 > CompressedIntAvg::MAX_NUM_PER_THREAD ||
               CompressedIntAvg::WillOverflow(CompressedIntAvg::ExtendSignBit(sum), val)))) {
        // Although agent->element might have been cleared at this
        // point, it is just OK because the very value is 0 in
        // this case
        wb->CommitAndClear();
        sum = 0;
        num = 0;
        n = 0;
      }
    } while (!wb->buffer_.CompareExchangeWeak(
        &n, CompressedIntAvg::Compress(num + 1, sum + complement)));
  }

  /// @brief Merge thread local into global.
  static void Merge(ResultType* wb1, add_cr_non_integral_t<Type> wb2) {
    MergeIntBuffer()(wb1, wb2);
  }
};

/// @brief For int type, use IntRecorder for better performance.
template <typename T, typename Base = WriteMostly<AvgTraits<T>>>
class Averager : public Base {
 public:
  Averager() : Base(AvgBuffer<T>{T(), 0}, AvgBuffer<T>{T(), 0}) {}

  explicit Averager(std::string_view rel_path) : Averager() {
    handle_ = Base::LinkToParent(rel_path);
  }

  Averager(TrpcVarGroup* parent, std::string_view rel_path) : Averager() {
    handle_ = Base::LinkToParent(rel_path, parent);
  }

  std::string GetAbsPath() const { return IsExposed() ? handle_->abs_path : std::string(); }

  bool IsExposed() const { return handle_.has_value(); }

 private:
  std::optional<TrpcVarGroup::Handle> handle_;
};

/// @brief Only support int type.
class IntRecorder : public WriteMostly<CompressedIntAvgTraits> {
 public:
  IntRecorder()
      : WriteMostly<CompressedIntAvgTraits>(CompressedIntAvgTraits::ResultType{0, 0}, 0) {}

  explicit IntRecorder(std::string_view rel_path) : IntRecorder() {
    handle_ = LinkToParent(rel_path);
  }

  IntRecorder(TrpcVarGroup* parent, std::string_view rel_path) : IntRecorder() {
    handle_ = LinkToParent(rel_path, parent);
  }

  std::string GetAbsPath() const { return IsExposed() ? handle_->abs_path : std::string(); }

  bool IsExposed() const { return handle_.has_value(); }

 private:
  std::optional<TrpcVarGroup::Handle> handle_;
};

// End of source codes that are from incubator-brpc.

}  // namespace trpc::tvar
