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

#include "test/end2end/unary/redis/redis_server.h"

#include "test/end2end/common/util.h"
#include "trpc/common/trpc_app.h"

namespace trpc::testing {

int RedisServer::Initialize() {
  server_->Start();

  test_signal_->SignalClientToContinue();

  return 0;
}

void RedisServer::Destroy() { GcovFlush(); }

}  // namespace trpc::testing
