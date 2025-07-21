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

#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/client_context.h"
#include "trpc/client/service_proxy.h"
#include "trpc/client/testing/mock_client_transport.h"
#include "trpc/codec/codec_manager.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/naming/common/util/loadbalance/polling/polling_load_balance.h"
#include "trpc/naming/direct/direct_selector_filter.h"
#include "trpc/naming/direct/selector_direct.h"
#include "trpc/naming/selector_factory.h"
#include "trpc/runtime/runtime.h"
#include "trpc/serialization/trpc_serialization.h"

namespace trpc::testing {

// Init the plugins required for unit tests
void RegisterPlugins() {
  // register direct selector
  auto direct_selector = MakeRefCounted<SelectorDirect>(MakeRefCounted<PollingLoadBalance>());
  direct_selector->Init();
  SelectorFactory::GetInstance()->Register(direct_selector);
  auto direct_selector_filter = std::make_shared<DirectSelectorFilter>();
  direct_selector_filter->Init();
  FilterManager::GetInstance()->AddMessageClientFilter(direct_selector_filter);

  // register codec
  codec::Init();
  serialization::Init();
  runtime::StartRuntime();
}

void UnregisterPlugins() {
  runtime::TerminateRuntime();
  serialization::Destroy();
  codec::Destroy();
  FilterManager::GetInstance()->Clear();
  SelectorFactory::GetInstance()->Clear();
}

class TestServiceProxy : public ServiceProxy {
 public:
  void SetServiceProxyOption(const std::shared_ptr<ServiceProxyOption>& option) {
    return SetServiceProxyOptionInner(option);
  }

  const std::string& GetServiceName() { return ServiceProxy::GetServiceName(); }

  void FillClientContext(const ClientContextPtr& ctx) { ServiceProxy::FillClientContext(ctx); }

  stream::StreamReaderWriterProviderPtr MockSelectStreamProvider(const ClientContextPtr& context, void* rpc_reply_msg) {
    return SelectStreamProvider(context, rpc_reply_msg);
  }

  int RunFilters(const FilterPoint& point, const ClientContextPtr& context) {
    return ServiceProxy::RunFilters(point, context);
  }
};

using TestServiceProxyPtr = std::shared_ptr<TestServiceProxy>;

}  // namespace trpc::testing
