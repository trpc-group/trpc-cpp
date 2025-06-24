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

/// @brief Direct service discovery filter
class DirectSelectorFilter : public MessageClientFilter {
 public:
  /// @brief Constructor that creates a SelectorWorkFlow object
  DirectSelectorFilter();

  /// @brief Initializes the SelectorWorkFlow object
  /// @return 0 if successful, -1 otherwise
  int Init() override {
    auto ret = selector_flow_->Init();
    if (!ret) {
      selector_flow_->SetGetServiceNameFunc(
          [](const ClientContextPtr& context) -> const std::string& { return context->GetServiceProxyOption()->name; });
    }
    return ret;
  }

  /// @brief Returns the name of the filter
  /// @return The name of the filter
  std::string Name() override { return "direct"; }

  /// @brief Returns the type of the filter
  /// @return The type of the filter
  FilterType Type() override { return FilterType::kSelector; }

  /// @brief Returns the filter points where the filter is triggered
  /// @return A vector of filter points
  std::vector<FilterPoint> GetFilterPoint() override {
    std::vector<FilterPoint> points = {FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_POST_RPC_INVOKE};
    return points;
  }

  /// @brief Runs the SelectorWorkFlow object
  /// @param status The filter status
  /// @param point The filter point
  /// @param context The client context
  void operator()(FilterStatus& status, FilterPoint point, const ClientContextPtr& context) override {
    selector_flow_->RunFilter(status, point, context);
  }

 private:
  std::unique_ptr<SelectorWorkFlow> selector_flow_;
};

}  // namespace trpc
