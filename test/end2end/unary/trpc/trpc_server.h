// Copyright (c) 2023, Tencent Inc.
// All rights reserved.

#pragma once

#include "test/end2end/common/test_signaller.h"
#include "trpc/common/trpc_app.h"

namespace trpc::testing {

/// @brief Server for trpc unary test
class TrpcTestServer : public ::trpc::TrpcApp {
 public:
  explicit TrpcTestServer(TestSignaller* test_signal) : test_signal_(test_signal) {}
  int Initialize() override;
  void Destroy() override;

 private:
  TestSignaller* test_signal_{nullptr};
};

}  // namespace trpc::testing
