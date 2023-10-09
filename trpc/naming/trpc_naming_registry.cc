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

#include "trpc/naming/trpc_naming_registry.h"

#include "trpc/common/config/trpc_config.h"
#include "trpc/filter/filter.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/naming/common/util/loadbalance/polling/polling_load_balance.h"
#include "trpc/naming/direct/direct_selector_filter.h"
#include "trpc/naming/direct/selector_direct.h"
#include "trpc/naming/domain/domain_selector_filter.h"
#include "trpc/naming/domain/selector_domain.h"
#include "trpc/naming/limiter.h"
#include "trpc/naming/limiter_factory.h"
#include "trpc/naming/load_balance_factory.h"
#include "trpc/naming/registry.h"
#include "trpc/naming/registry_factory.h"
#include "trpc/naming/selector.h"
#include "trpc/naming/selector_factory.h"

namespace trpc {

// Initialize the Selector inside trpc: currently domain and direct plugins.
void RegisterInnerSelector() {
  LoadBalancePtr polling_load_balance = trpc::LoadBalanceFactory::GetInstance()->Get(kPollingLoadBalance);
  if (polling_load_balance == nullptr) {
    // Register the default load balancer
    polling_load_balance = MakeRefCounted<PollingLoadBalance>();
    LoadBalanceFactory::GetInstance()->Register(polling_load_balance);
  }

  SelectorPtr domain_selector = MakeRefCounted<SelectorDomain>(polling_load_balance);
  SelectorFactory::GetInstance()->Register(domain_selector);

  SelectorPtr direct_selector = MakeRefCounted<SelectorDirect>(polling_load_balance);
  SelectorFactory::GetInstance()->Register(direct_selector);

  // Because other plugins are registered using the direct/domain selector filter,
  // so direct and domain selector filter need to be registered when naming module is initialized
  MessageClientFilterPtr direct_selector_filter = std::make_shared<DirectSelectorFilter>();
  direct_selector_filter->Init();
  FilterManager::GetInstance()->AddMessageClientFilter(direct_selector_filter);

  MessageClientFilterPtr domain_selector_filter = std::make_shared<DomainSelectorFilter>();
  domain_selector_filter->Init();
  FilterManager::GetInstance()->AddMessageClientFilter(domain_selector_filter);
}

// Stop the Selector thread inside trpc: currently contains domain and direct plugins.
void StopInnerSelector() {
  auto selector_ptr = SelectorFactory::GetInstance()->Get("direct");
  if (selector_ptr) {
    selector_ptr->Stop();
  }

  selector_ptr = SelectorFactory::GetInstance()->Get("domain");
  if (selector_ptr) {
    selector_ptr->Stop();
  }
}

// Destroy Selector inside trpc: currently contains domain and direct plugins.
void UnregisterInnerSelector() {
  auto selector_ptr = SelectorFactory::GetInstance()->Get("direct");
  if (selector_ptr) {
    selector_ptr->Destroy();
  }

  selector_ptr = SelectorFactory::GetInstance()->Get("domain");
  if (selector_ptr) {
    selector_ptr->Destroy();
  }
}

// unregister the selector plugin
int UnregisterSelector() {
  SelectorFactory::GetInstance()->Clear();

  // release load balancers
  LoadBalanceFactory::GetInstance()->Clear();

  return 0;
}

// unregister the registry plugin
int UnregistryRegistry() {
  RegistryFactory::GetInstance()->Clear();
  return 0;
}

// Unregister the limiter plugin
int UnregiterLimiter() {
  LimiterFactory::GetInstance()->Clear();
  return 0;
}

namespace naming {

bool Init() {
  // Register the inner selector
  RegisterInnerSelector();

  return true;
}

void Stop() {}

void Destroy() {
  UnregiterLimiter();

  UnregistryRegistry();

  UnregisterSelector();
}

}  // namespace naming

}  // namespace trpc
