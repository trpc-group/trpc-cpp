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
#include <string>

#include "trpc/runtime/iomodel/reactor/common/accept_connection_info.h"
#include "trpc/transport/server/fiber/fiber_bind_adapter.h"
#include "trpc/transport/server/server_transport.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

/// @brief Server transport implement for fiber threadmodel
class FiberServerTransportImpl final : public ServerTransport {
 public:
  ~FiberServerTransportImpl() override;

  std::string Name() const override { return "fiber"; }

  std::string Version() const override { return "0.1.0"; }

  void Bind(const BindInfo& bind_info) override;

  bool Listen() override;

  void Stop() override;

  void StopListen(bool clean_conn) override;

  void Destroy() override;

  int SendMsg(STransportRspMsg* msg) override;

  bool AcceptConnection(AcceptConnectionInfo& connection_info);

  BindInfo& GetBindInfo() { return bind_info_; }

  void DecrAliveConnNum(int size) { alive_conns_.fetch_sub(size, std::memory_order_relaxed); }

  bool IsConnected(uint64_t connection_id) override;

  void DoClose(const CloseConnectionInfo& close_connection_info) override;

 private:
  std::vector<RefPtr<FiberBindAdapter>> bind_adapters_;

  BindInfo bind_info_;

  std::atomic<int> alive_conns_{0};
};

}  // namespace trpc
