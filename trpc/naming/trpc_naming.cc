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

#include "trpc/naming/trpc_naming.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/naming/limiter.h"
#include "trpc/naming/limiter_factory.h"
#include "trpc/naming/registry.h"
#include "trpc/naming/registry_factory.h"
#include "trpc/naming/selector.h"
#include "trpc/naming/selector_factory.h"

namespace trpc::naming {

int Register(const TrpcRegistryInfo& info) {
  const std::string& plugin_name = info.plugin_name;
  if (plugin_name.empty()) {
    return -1;
  }

  RegistryPtr ptr = RegistryFactory::GetInstance()->Get(plugin_name);
  if (!ptr) {
    return -1;
  }

  return ptr->Register(&info.registry_info);
}

int Unregister(const TrpcRegistryInfo& info) {
  const std::string& plugin_name = info.plugin_name;
  if (plugin_name.empty()) {
    return -1;
  }

  RegistryPtr ptr = RegistryFactory::GetInstance()->Get(plugin_name);
  if (!ptr) {
    return -1;
  }

  return ptr->Unregister(&info.registry_info);
}

int HeartBeat(const TrpcRegistryInfo& info) {
  const std::string& plugin_name = info.plugin_name;
  if (plugin_name.empty()) {
    return -1;
  }

  RegistryPtr ptr = RegistryFactory::GetInstance()->Get(plugin_name);
  if (!ptr) {
    return -1;
  }

  return ptr->HeartBeat(&info.registry_info);
}

Future<> AsyncHeartBeat(const TrpcRegistryInfo& info) {
  const std::string& plugin_name = info.plugin_name;
  if (plugin_name.empty()) {
    return MakeExceptionFuture<>(CommonException("plugin name no set"));
  }

  RegistryPtr ptr = RegistryFactory::GetInstance()->Get(plugin_name);
  if (!ptr) {
    return MakeExceptionFuture<>(CommonException("plugin no found"));
  }

  return ptr->AsyncHeartBeat(&info.registry_info);
}

int Select(const TrpcSelectorInfo& info, TrpcEndpointInfo& endpoint) {
  const std::string& plugin_name = info.plugin_name;
  if (plugin_name.empty()) {
    return -1;
  }

  SelectorPtr ptr = SelectorFactory::GetInstance()->Get(plugin_name);
  if (!ptr) {
    return -1;
  }

  return ptr->Select(&info.selector_info, &endpoint);
}

Future<TrpcEndpointInfo> AsyncSelect(const TrpcSelectorInfo& info) {
  const std::string& plugin_name = info.plugin_name;
  if (plugin_name.empty()) {
    return MakeExceptionFuture<TrpcEndpointInfo>(CommonException("plugin name no set"));
  }

  SelectorPtr ptr = SelectorFactory::GetInstance()->Get(plugin_name);
  if (!ptr) {
    return MakeExceptionFuture<TrpcEndpointInfo>(CommonException("plugin no found"));
  }

  return ptr->AsyncSelect(&info.selector_info);
}

int SelectBatch(const trpc::TrpcSelectorInfo& info, std::vector<trpc::TrpcEndpointInfo>& endpoints) {
  const std::string& plugin_name = info.plugin_name;
  if (plugin_name.empty()) {
    return -1;
  }

  SelectorPtr ptr = SelectorFactory::GetInstance()->Get(plugin_name);
  if (!ptr) {
    return -1;
  }

  return ptr->SelectBatch(&info.selector_info, &endpoints);
}

Future<std::vector<TrpcEndpointInfo>> AsyncSelectBatch(const TrpcSelectorInfo& info) {
  const std::string& plugin_name = info.plugin_name;
  if (plugin_name.empty()) {
    return MakeExceptionFuture<std::vector<TrpcEndpointInfo>>(CommonException("plugin name no set"));
  }

  SelectorPtr ptr = SelectorFactory::GetInstance()->Get(plugin_name);
  if (!ptr) {
    return MakeExceptionFuture<std::vector<TrpcEndpointInfo>>(CommonException("plugin no found"));
  }

  return ptr->AsyncSelectBatch(&info.selector_info);
}

int ReportInvokeResult(const TrpcInvokeResult& result) {
  const std::string& plugin_name = result.plugin_name;
  if (plugin_name.empty()) {
    return -1;
  }

  SelectorPtr ptr = SelectorFactory::GetInstance()->Get(plugin_name);
  if (!ptr) {
    return -1;
  }

  return ptr->ReportInvokeResult(&result.invoke_result);
}

bool SetCircuitBreakWhiteList(const TrpcCircuitBreakWhiteList& white_list) {
  const std::string& plugin_name = white_list.plugin_name;
  if (plugin_name.empty()) {
    return false;
  }

  SelectorPtr ptr = SelectorFactory::GetInstance()->Get(plugin_name);
  if (!ptr) {
    return false;
  }

  return ptr->SetCircuitBreakWhiteList(white_list.framework_retcodes);
}

int SetEndpoints(const TrpcRouterInfo& info) {
  const std::string& plugin_name = info.plugin_name;
  if (plugin_name.empty()) {
    return -1;
  }

  SelectorPtr ptr = SelectorFactory::GetInstance()->Get(plugin_name);
  if (!ptr) {
    return -1;
  }

  return ptr->SetEndpoints(&info.router_info);
}

LimitRetCode ShouldLimit(const TrpcLimitInfo& info) {
  LimiterPtr ptr = LimiterFactory::GetInstance()->Get(info.plugin_name);
  if (!ptr) {
    return LimitRetCode::kLimitError;
  }

  return ptr->ShouldLimit(&(info.limit_info));
}

int FinishLimit(const TrpcLimitResult& result) {
  LimiterPtr ptr = LimiterFactory::GetInstance()->Get(result.plugin_name);
  if (!ptr) {
    return -1;
  }

  return ptr->FinishLimit(&(result.limit_result));
}

}  // namespace trpc::naming
