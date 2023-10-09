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

#include "test/end2end/unary/redis/redis_server.h"

#include "test/end2end/common/util.h"
#include "trpc/common/trpc_app.h"

namespace trpc::testing {

int RedisServer::Initialize() {
  server_->Start();

  test_signal_->SignalClientToContinue();

  return 0;
}

extern "C" void __gcov_flush();

void RedisServer::Destroy() { __gcov_flush(); }

}  // namespace trpc::testing
