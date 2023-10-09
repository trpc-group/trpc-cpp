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

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "trpc/client/client_context.h"
#include "trpc/filter/filter.h"
#include "trpc/rpcz/span.h"
#include "trpc/rpcz/util/sampler.h"

namespace trpc::rpcz {

/// @brief Sample function type of client rpcz filter.
using CustomerClientRpczSampleFunction = std::function<bool(const trpc::ClientContextPtr& context)>;

// Handle function type of client filter point.
/// @private
using ClientFilterPointHandleFunction = std::function<void(const trpc::ClientContextPtr& context)>;

/// @brief Client filter to record rpcz info for pure client scennario.
/// @private
class RpczClientFilter : public trpc::MessageClientFilter {
 public:
  /// @brief Default constructor.
  RpczClientFilter();

  /// @brief Default destructor.
  ~RpczClientFilter() override = default;

  /// @brief Get filter name.
  std::string Name() override { return "rpcz"; }

  /// @brief Which filter points to run on.
  std::vector<FilterPoint> GetFilterPoint() override;

  /// @brief Client filter entry function for framework to invoke, routine goes into rpcz filter.
  void operator()(FilterStatus& status, FilterPoint point, const ClientContextPtr& context) override;

  /// @brief Invoked by user to set sample funtion, which decide whether to generate span for current request.
  /// @param sample_function User-defined sample function.
  void SetSampleFunction(const CustomerClientRpczSampleFunction& sample_function);

  /// @brief Delete user set sample function.
  /// @note Not thread-safe.
  void DelSampleFunction();

  /// @brief Get user-defined sample function.
  CustomerClientRpczSampleFunction GetSampleFunction() { return customer_rpcz_sample_function_; }

 private:
  /// @brief To decide whether to sample current request.
  /// @param context Client context.
  /// @param span_id Global span id of current request.
  /// @return True: do sample, false: not sample.
  bool ShouldSample(const trpc::ClientContextPtr& context, uint32_t span_id);

  /// @brief Run high low water sampler to find out whether sample or not.
  /// @param span_id Global span id of current request.
  /// @return True: do sample, false: not sample.
  bool HighLowWaterSample(uint32_t span_id);

  /// @brief Run in first filter point.
  /// @param context Client conetxt.
  void PreRpcHandle(const trpc::ClientContextPtr& context);

  /// @brief Run in second filter point.
  /// @param context Client conetxt.
  void PreSendHandle(const trpc::ClientContextPtr& context);

  /// @brief Run in third filter point, before request push into io-handle queue.
  /// @param context Client conetxt.
  void PreSchedSendHandle(const trpc::ClientContextPtr& context);

  /// @brief Run in fourth filter point, after request push into io-handle queue.
  /// @param context Client conetxt.
  void PostSchedSendHandle(const trpc::ClientContextPtr& context);

  /// @brief Run in fifth filter point, after request sent into network.
  /// @param context Client conetxt.
  void PostIoSendHandle(const trpc::ClientContextPtr& context);

  /// @brief Run in sixth filter point, after reveive response.
  /// @param context Client conetxt.
  void PreSchedRecvHandle(const trpc::ClientContextPtr& context);

  /// @brief Run in seventh filter point, after response out of queue.
  /// @param context Client conetxt.
  void PostSchedRecvHandle(const trpc::ClientContextPtr& context);

  /// @brief Run in eighth filter point, into transport.
  /// @param context Client conetxt.
  void PostRecvHandle(const trpc::ClientContextPtr& context);

  /// @brief Run in ninth filter point, into user callback.
  /// @param context Client conetxt.
  void PostRpcHandle(const trpc::ClientContextPtr& context);

  /// @brief To init span from client context.
  /// @param context Client context.
  /// @param span Span generated in route scenario.
  /// @note Used in route scenario.
  void FillContextToRouteSpan(const trpc::ClientContextPtr& context, trpc::rpcz::Span* span);

 private:
  // Whether user-defined sample function set.
  bool customer_func_set_flag_;
  // User-defined function.
  CustomerClientRpczSampleFunction customer_rpcz_sample_function_;
  // Framework default sampler.
  HighLowWaterLevelSampler high_low_water_level_sampler_;
  // All the handle functions to run in filter points.
  std::map<trpc::FilterPoint, ClientFilterPointHandleFunction> point_handles_;
};

}  // namespace trpc::rpcz
#endif
