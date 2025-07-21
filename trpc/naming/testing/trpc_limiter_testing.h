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

#include "trpc/naming/limiter.h"

namespace trpc::testing {

class TestTrpcLimiter : public Limiter {
 public:
  TestTrpcLimiter() = default;

  ~TestTrpcLimiter() override = default;

  std::string Name() const { return "test_trpc_limiter"; };

  std::string Version() const { return "0.0.1"; };

  int Init() noexcept override { return 0; }

  void Start() noexcept override {}

  void Stop() noexcept override {}

  void Destroy() noexcept override {}

  // Synchronously get throtting state interface
  LimitRetCode ShouldLimit(const LimitInfo* info) override { return LimitRetCode::kLimitOK; }

  // End the throttling processing interface, such as reporting some throttling data, etc
  int FinishLimit(const LimitResult* result) override { return 0; }
};

}  // namespace trpc::testing
