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

#include "test/end2end/common/test_signaller.h"
#include "trpc/common/trpc_app.h"

namespace trpc::testing {

class FutureServer: public trpc::TrpcApp {
 public:
  explicit FutureServer(TestSignaller* test_signal) : test_signal_(test_signal) {}
  int Initialize() override;
  void Destroy() override;

 private:
  TestSignaller* test_signal_{nullptr};
};

}  // namespace trpc::testing
