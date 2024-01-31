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

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "trpc/filter/filter.h"
#include "trpc/naming/selector_workflow.h"

namespace trpc {

/// @brief DNS discovery filter
class DomainSelectorFilter : public MessageClientFilter {
 public:
  DomainSelectorFilter();

  /// @brief initialization
  int Init() override {
    auto ret = selector_flow_->Init();
    if (!ret) {
      selector_flow_->SetGetServiceNameFunc(
          [](const ClientContextPtr& context) -> const std::string& { return context->GetServiceProxyOption()->name; });
    }
    return ret;
  }

  /// @brief plugin name
  std::string Name() override { return "domain"; }

  /// @brief plugin type
  FilterType Type() override { return FilterType::kSelector; }

  /// @brief triggers the corresponding processing at the tracking point
  std::vector<FilterPoint> GetFilterPoint() override {
    std::vector<FilterPoint> points = {FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_POST_RPC_INVOKE};
    return points;
  }

  void operator()(FilterStatus& status, FilterPoint point, const ClientContextPtr& context) override {
    selector_flow_->RunFilter(status, point, context);
  }

 private:
  std::unique_ptr<SelectorWorkFlow> selector_flow_;
};

}  // namespace trpc
