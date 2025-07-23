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

#include "trpc/filter/trpc_filter.h"

#include <string>

#include "trpc/client/make_client_context.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/filter/filter_manager.h"
#ifdef TRPC_BUILD_INCLUDE_PROMETHEUS
#include "trpc/metrics/prometheus/prometheus_client_filter.h"
#include "trpc/metrics/prometheus/prometheus_server_filter.h"
#endif
#ifdef TRPC_BUILD_INCLUDE_RPCZ
#include "trpc/rpcz/filter/client_filter.h"
#include "trpc/rpcz/filter/rpcz_filter_index.h"
#include "trpc/rpcz/filter/server_filter.h"
#include "trpc/rpcz/span.h"
#endif
#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL
#include "trpc/overload_control/concurrency_limiter/concurrency_limiter_server_filter.h"
#include "trpc/overload_control/fiber_limiter/fiber_limiter_client_filter.h"
#include "trpc/overload_control/fiber_limiter/fiber_limiter_server_filter.h"
#include "trpc/overload_control/high_percentile/high_percentile_server_filter.h"
#include "trpc/overload_control/throttler/throttler_client_filter.h"
#include "trpc/overload_control/flow_control/flow_controller_server_filter.h"
#endif

// #include "trpc/filter/retry/retry_limit_client_filter.h"
// #include "trpc/overload_control/flow_control/flow_controller_factory.h"

namespace trpc::filter {

namespace {

bool InitializeServerFilter() {
  // 1. Registers the default server filter which provided by the framework.
#ifdef TRPC_BUILD_INCLUDE_PROMETHEUS
  FilterManager::GetInstance()->AddMessageServerFilter(std::make_shared<PrometheusServerFilter>());
#endif

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL
  MessageServerFilterPtr concurrency_limiter_server_filter(new overload_control::ConcurrencyLimiterServerFilter());
  concurrency_limiter_server_filter->Init();
  FilterManager::GetInstance()->AddMessageServerFilter(concurrency_limiter_server_filter);

  MessageServerFilterPtr fiber_limiter_server_filter(new overload_control::FiberLimiterServerFilter());
  fiber_limiter_server_filter->Init();
  FilterManager::GetInstance()->AddMessageServerFilter(fiber_limiter_server_filter);

  MessageServerFilterPtr high_percentile_server_filter(new overload_control::HighPercentileServerFilter());
  high_percentile_server_filter->Init();
  FilterManager::GetInstance()->AddMessageServerFilter(high_percentile_server_filter);

  MessageServerFilterPtr flow_control_server_filter(new overload_control::FlowControlServerFilter());
  flow_control_server_filter->Init();
  FilterManager::GetInstance()->AddMessageServerFilter(flow_control_server_filter);
#endif


#ifdef TRPC_BUILD_INCLUDE_RPCZ
  FilterManager::GetInstance()->AddMessageServerFilter(std::make_shared<trpc::rpcz::RpczServerFilter>());
  // Set callback here as only used by rpcz proxy scenario.
  trpc::RegisterMakeClientContextCallback([](const ServerContextPtr& ctx, ClientContextPtr& client_ctx) {
    auto* ptr = ctx->GetFilterData<rpcz::ServerRpczSpan>(rpcz::kTrpcServerRpczIndex);
    if (!ptr) return;

    rpcz::ClientRouteRpczSpan client_route_rpcz_span;
    client_route_rpcz_span.sample_flag = ptr->sample_flag;
    client_route_rpcz_span.parent_span = ptr->span;
    client_ctx->SetFilterData<rpcz::ClientRouteRpczSpan>(rpcz::kTrpcRouteRpczIndex, std::move(client_route_rpcz_span));
  });
#endif

  // 2. Initializes all the filters which had been registered
  for (const auto& [name, server_filter] : FilterManager::GetInstance()->GetAllMessageServerFilters()) {
    if (server_filter->Init() != 0) {
      TRPC_FMT_ERROR("Init of {} server filter failed!", name);
      return false;
    }
  }

  // 3. Put filters into the FilterManager in the order specified by the configuration.
  std::vector<std::string> filter_names = trpc::TrpcConfig::GetInstance()->GetServerConfig().filters;
  for (auto& filter_name : filter_names) {
    auto filter = FilterManager::GetInstance()->GetMessageServerFilter(filter_name);
    if (filter != nullptr) {
      FilterManager::GetInstance()->AddMessageServerGlobalFilter(filter);
    }
  }

  return true;
}

bool InitializeClientFilter() {
  // 1. Registers the default client filter which provided by the framework.
#ifdef TRPC_BUILD_INCLUDE_PROMETHEUS
  FilterManager::GetInstance()->AddMessageClientFilter(std::make_shared<PrometheusClientFilter>());
#endif

#ifdef TRPC_BUILD_INCLUDE_RPCZ
  FilterManager::GetInstance()->AddMessageClientFilter(std::make_shared<trpc::rpcz::RpczClientFilter>());
#endif

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL
  MessageClientFilterPtr fiber_limiter_client_filter(new overload_control::FiberLimiterClientFilter());
  fiber_limiter_client_filter->Init();
  FilterManager::GetInstance()->AddMessageClientFilter(fiber_limiter_client_filter);

  MessageClientFilterPtr throttler_client_filter(new overload_control::ThrottlerClientFilter());
  throttler_client_filter->Init();
  FilterManager::GetInstance()->AddMessageClientFilter(throttler_client_filter);
#endif

  // 2. Initializes all the filters which had been registered
  for (const auto& [name, client_filter] : FilterManager::GetInstance()->GetAllMessageClientFilters()) {
    if (client_filter->Init() != 0) {
      TRPC_FMT_ERROR("Init of {} client filter failed!", name);
      return false;
    }
  }

  // 3. Put filters into the FilterManager in the order specified by the configuration.
  std::vector<std::string> filter_names = trpc::TrpcConfig::GetInstance()->GetClientConfig().filters;
  for (auto& filter_name : filter_names) {
    auto filter = FilterManager::GetInstance()->GetMessageClientFilter(filter_name);
    if (filter != nullptr) {
      FilterManager::GetInstance()->AddMessageClientGlobalFilter(filter);
    }
  }

  return true;
}

}  // namespace

bool Init() {
  if (!InitializeServerFilter()) {
    return false;
  }

  if (!InitializeClientFilter()) {
    return false;
  }

  return true;
}

}  // namespace trpc::filter
