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

#include "trpc/naming/selector.h"

namespace trpc::testing {

class TestTrpcSelector : public trpc::Selector {
 public:
  TestTrpcSelector() = default;

  ~TestTrpcSelector() override = default;

  std::string Name() const override { return "test_trpc_selector"; }

  std::string Version() const override { return "0.0.1"; }

  int Init() noexcept override { return 0; }

  void Start() noexcept override {}

  void Stop() noexcept override {}

  void Destroy() noexcept override {}

  int Select(const SelectorInfo* info, TrpcEndpointInfo* endpoint) override { return 0; }

  Future<TrpcEndpointInfo> AsyncSelect(const SelectorInfo* info) override {
    TrpcEndpointInfo endpoint;
    return MakeReadyFuture<TrpcEndpointInfo>(std::move(endpoint));
  }

  int SelectBatch(const SelectorInfo* info, std::vector<TrpcEndpointInfo>* endpoints) override { return 0; }

  Future<std::vector<TrpcEndpointInfo>> AsyncSelectBatch(const SelectorInfo* info) override {
    std::vector<TrpcEndpointInfo> endpoint;
    return MakeReadyFuture<std::vector<TrpcEndpointInfo>>(std::move(endpoint));
  }

  int ReportInvokeResult(const InvokeResult* result) override { return 0; }

  int SetEndpoints(const RouterInfo* info) override { return 0; }
};

}  // namespace trpc::testing
