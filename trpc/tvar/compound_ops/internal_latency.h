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
//
// USERS DO NOT USE THIS FILE DIRECTLY, IT IS FOR INTERNAL USE!

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "json/json.h"
#include "trpc/tvar/basic_ops/reducer.h"
#include "trpc/tvar/common/tvar_group.h"
#include "trpc/tvar/compound_ops/window.h"
#include "trpc/util/chrono/tsc.h"

namespace trpc::tvar::internal {

struct IdentityTime {
  uint64_t operator()(const uint64_t& val) const { return val; }
};

struct TscToDuration {
  std::uint64_t operator()(const uint64_t& val) const {
    return DurationFromTsc(0, val) / std::chrono::nanoseconds(1);
  }
};

template <typename F = IdentityTime>
class InternalLatency {
 public:
  explicit InternalLatency(std::string_view rel_path) {
    LinkToParent(rel_path, TrpcVarGroup::FindOrCreate("/"));
  }

  InternalLatency(TrpcVarGroup* parent, std::string_view rel_path) {
    LinkToParent(rel_path, parent);
  }

  void Update(uint64_t v) {
    min_.Update(v);
    max_.Update(v);
    sum_.Add(v);
    cnt_.Increment();
  }

  bool IsExposed() const { return handle_.has_value(); }

 private:
  Json::Value ToJsonValue(const InternalLatency<F>& v) {
    Json::Value result;
    {
      auto cnt = v.cnt_.GetValue();
      auto sum = v.sum_.GetValue();
      result["total"]["avg"] = detail::ToJsonValue(F()(cnt != 0 ? (sum / cnt) : cnt));
      result["total"]["cnt"] = static_cast<Json::UInt64>(cnt);
    }
    static const std::vector<std::pair<std::string, time_t>> m = {
        {"1s", 1}, {"1min", 60}, {"10min", 600}, {"30min", 1800}, {"1h", 3600}};
    for (auto&& [k, r] : m) {
      auto cnt = v.win_cnt_.GetValue(r);
      auto sum = v.win_sum_.GetValue(r);
      result[k]["avg"] = detail::ToJsonValue(F()(cnt != 0 ? (sum / cnt) : cnt));
      result[k]["min"] = detail::ToJsonValue(F()(v.win_min_.GetValue(r)));
      result[k]["max"] = detail::ToJsonValue(F()(v.win_max_.GetValue(r)));
      result[k]["cnt"] = static_cast<Json::UInt64>(cnt);
    }
    return result;
  }

  void LinkToParent(std::string_view rel_path, TrpcVarGroup* parent) {
    handle_ = parent->Add(rel_path, [this] { return ToJsonValue(*this); });
  }

 private:
  Miner<uint64_t> min_{};
  Maxer<uint64_t> max_{};
  Counter<uint64_t> cnt_{};
  Counter<uint64_t> sum_{};
  Window<Miner<uint64_t>> win_min_{&min_, 3600};
  Window<Maxer<uint64_t>> win_max_{&max_, 3600};
  Window<Counter<uint64_t>> win_cnt_{&cnt_, 3600};
  Window<Counter<uint64_t>> win_sum_{&sum_, 3600};

  std::optional<TrpcVarGroup::Handle> handle_;
};

using InternalLatencyInTsc = InternalLatency<TscToDuration>;

}  // namespace trpc::tvar::internal
