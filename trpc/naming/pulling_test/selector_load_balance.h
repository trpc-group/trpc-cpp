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

#pragma once

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "trpc/common/config/load_balance_naming_conf.h"
#include "trpc/common/config/load_balance_naming_conf_parser.h"

#include "trpc/common/plugin.h"
#include "trpc/naming/common/util/utils_help.h"
#include "trpc/naming/load_balance.h"
#include "trpc/naming/selector.h"

namespace trpc {

/// @brief Polling Load Balance Test Class
class TestPollingLoadBalance : public Selector {
 public:
  explicit TestPollingLoadBalance(const LoadBalancePtr& load_balance);

  /// @brief 插件名称
  std::string Name() const override { return "test_polling_load_balance"; }

  /// @brief 插件版本
  std::string Version() const override { return "0.0.1"; }

  /// @brief 初始化
  /// @return 成功返回0，失败返回-1
  int Init() noexcept override;

  /// @brief 获取被调用服务的一个节点的路由信息
  int Select(const SelectorInfo* info, TrpcEndpointInfo* endpoint) override;

  /// @brief 异步获取被调用节点
  Future<TrpcEndpointInfo> AsyncSelect(const SelectorInfo* info) override;

  /// @brief 按策略批量获取节点路由信息
  int SelectBatch(const SelectorInfo* info, std::vector<TrpcEndpointInfo>* endpoints) override;

  /// @brief 异步按策略批量获取节点路由信息
  Future<std::vector<TrpcEndpointInfo>> AsyncSelectBatch(const SelectorInfo* info) override;

  /// @brief 报告调用结果的接口
  int ReportInvokeResult(const InvokeResult* result) override;

  /// @brief 设置被调用服务的路由信息的接口
  int SetEndpoints(const RouterInfo* info) override;
  naming::LoadBalanceSelectorConfig loadbalance_config_;
  LoadBalance* GetLoadBalance(const std::string& name);

 private:
  struct EndpointsInfo {
    // 节点信息
    std::vector<TrpcEndpointInfo> endpoints;
    // 节点ID生成器
    EndpointIdGenerator id_generator;
  };

 private:
  std::unordered_map<std::string, EndpointsInfo> targets_map_;
  LoadBalancePtr default_load_balance_ = nullptr;
  std::shared_mutex mutex_;
};
using TestPollingLoadBalancePtr = RefPtr<TestPollingLoadBalance>;
}  // namespace trpc
