// Copyright (c) 2023, Tencent Inc.
// All rights reserved.

#pragma once

#include "test/end2end/common/test_signaller.h"
#include "trpc/common/trpc_app.h"

namespace trpc::testing {

/// @brief RouteServer for trpc unary test
class RouteTestServer : public ::trpc::TrpcApp {
 public:
  explicit RouteTestServer(TestSignaller* test_signal) : test_signal_(test_signal) {}
  int Initialize() override;
  void Destroy() override;

 private:
  TestSignaller* test_signal_{nullptr};
};

}  // namespace trpc::testing
