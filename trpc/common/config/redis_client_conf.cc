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

#include "trpc/common/config/redis_client_conf.h"

#include "trpc/util/log/logging.h"

namespace trpc {

void RedisClientConf::Display() const {
  TRPC_LOG_DEBUG("redis_password:" << password);
  TRPC_LOG_DEBUG("redis user name:" << user_name);
  TRPC_LOG_DEBUG("redis_db:" << db);
}

}  // namespace trpc
