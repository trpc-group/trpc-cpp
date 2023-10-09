// Copyright (c) 2023, Tencent Inc.
// All rights reserved.

#pragma once

#include "test/end2end/common/test_signaller.h"
#include "trpc/common/trpc_app.h"

namespace trpc::testing {

/// @brief Streaming server (using synchronous stream interface).
class SyncStreamServer : public ::trpc::TrpcApp {
 public:
  explicit SyncStreamServer(TestSignaller* test_signal) : test_signal_(test_signal) {}
  int Initialize() override;
  void Destroy() override;

 private:
  TestSignaller* test_signal_{nullptr};
};

}  // namespace trpc::testing
