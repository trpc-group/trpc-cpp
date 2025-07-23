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
//

#ifdef TRPC_BUILD_INCLUDE_RPCZ
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "trpc/common/config/global_conf.h"
#include "trpc/filter/filter.h"
#include "trpc/rpcz/util/sampler.h"
#include "trpc/server/server_context.h"

namespace trpc::rpcz {

/// @brief Sample function type of server rpcz filter.
using CustomerServerRpczSampleFunction = std::function<bool(const trpc::ServerContextPtr& context)>;

/// Handle function type of server filter point.
/// @private
using ServerFilterPointHandleFunction = std::function<void(const trpc::ServerContextPtr& context)>;

/// @brief Server filter to record rpcz info for server scennario.
/// @private
class RpczServerFilter : public trpc::MessageServerFilter {
 public:
  /// @brief Default constructor.
  RpczServerFilter();

  /// @brief Default destructor.
  ~RpczServerFilter() override = default;

  /// @brief Get filter name.
  std::string Name() override { return "rpcz"; }

  /// @brief Which filter points to run on.
  std::vector<FilterPoint> GetFilterPoint() override;

  /// @brief Server filter entry function for framework to invoke, routine goes into rpcz filter.
  void operator()(FilterStatus& status, FilterPoint point, const ServerContextPtr& context) override;

  /// @brief Invoked by user to set sample funtion, which decide whether to generate span for current request.
  /// @param sample_function User-defined sample function.
  void SetSampleFunction(const CustomerServerRpczSampleFunction& sample_function);

  /// @brief Delete user set sample function.
  /// @note Not thread-safe.
  void DelSampleFunction();

  /// @brief Get user-defined sample function.
  CustomerServerRpczSampleFunction GetSampleFunction() { return customer_rpcz_sample_function_; }

 private:
  /// @brief To decide whether to sample current request.
  /// @param context Server context.
  /// @param span_id Global span id of current request.
  /// @return True: do sample, false: not sample.
  bool ShouldSample(const trpc::ServerContextPtr& context, uint32_t span_id);

  /// @brief Run high low water sampler to find out whether sample or not.
  /// @param span_id Global span id of current request.
  /// @return True: do sample, false: not sample.
  bool HighLowWaterSample(uint32_t span_id);

  /// @brief Run in first filter point, after receive raw request from kernel, before into recv queue.
  /// @param context Server conetxt.
  void PreSchedRecvHandle(const trpc::ServerContextPtr& context);

  /// @brief Run in second filter point, after out of recv queue.
  /// @param context Server conetxt.
  void PostSchedRecvHandle(const trpc::ServerContextPtr& context);

  /// @brief Run in third filter point, before invoke user callback.
  /// @param context Server conetxt.
  void PreRpcHandle(const trpc::ServerContextPtr& context);

  /// @brief Run in fourth filter point, after user callback finish.
  /// @param context Server conetxt.
  void PostRpcHandle(const trpc::ServerContextPtr& context);

  /// @brief Run in fifth filter point, before encode respnse.
  /// @param context Server conetxt.
  void PreSendHandle(const trpc::ServerContextPtr& context);

  /// @brief Run in sixth filter point, before into send queue.
  /// @param context Server conetxt.
  void PreSchedSendHandle(const trpc::ServerContextPtr& context);

  /// @brief Run in seventh filter point, after response out of send queue.
  /// @param context Server conetxt.
  void PostSchedSendHandle(const trpc::ServerContextPtr& context);

  /// @brief Run in eighth filter point, after response send into kernel.
  /// @param context Server conetxt.
  void PostIoSendHandle(const trpc::ServerContextPtr& context);

 private:
  // Whether user-defined sample function set.
  bool customer_func_set_flag_;
  // User-defined function.
  CustomerServerRpczSampleFunction customer_rpcz_sample_function_;
  //  Framework default sampler.
  HighLowWaterLevelSampler high_low_water_level_sampler_;
  // All the handle functions to run in filter points.
  std::map<trpc::FilterPoint, ServerFilterPointHandleFunction> point_handles_;
};

}  // namespace trpc::rpcz
#endif
