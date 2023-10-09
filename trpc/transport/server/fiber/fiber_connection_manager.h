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

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "trpc/runtime/iomodel/reactor/fiber/fiber_tcp_connection.h"
#include "trpc/util/align.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

class FiberConnectionManager {
 public:
  FiberConnectionManager();

  ~FiberConnectionManager();

  void Add(uint64_t conn_id, RefPtr<FiberTcpConnection>&& conn);

  RefPtr<FiberTcpConnection> Del(uint64_t conn_id);

  RefPtr<FiberTcpConnection> Get(uint64_t conn_id);

  void GetIdles(uint32_t idle_timeout, std::vector<RefPtr<FiberTcpConnection>>& idle_conns);

  void Stop();

  void Destroy();

  void DisableAllConnRead();

 private:
  struct alignas(hardware_destructive_interference_size) ConnectionShard {
    std::mutex lock;
    std::unordered_map<uint64_t, RefPtr<FiberTcpConnection>> map;
  };

  constexpr static size_t kShards = 128;

  std::unique_ptr<ConnectionShard[]> conn_shards_;
};

}  // namespace trpc
