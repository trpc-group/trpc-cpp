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

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "trpc/runtime/iomodel/reactor/common/accept_connection_info.h"
#include "trpc/runtime/iomodel/reactor/reactor.h"
#include "trpc/transport/server/server_transport.h"

namespace trpc {

class BindAdapter;

/// @brief Server transport implement for separate/merge threadmodel
class ServerTransportImpl final : public ServerTransport {
 public:
  explicit ServerTransportImpl(std::vector<Reactor*>&& reactors);

  ~ServerTransportImpl() override;

  std::string Name() const override { return "default"; }

  std::string Version() const override { return "0.1.0"; }

  void Bind(const BindInfo& bind_info) override;

  bool Listen() override;

  void Stop() override;

  void StopListen(bool clean_conn) override;

  void Destroy() override;

  int SendMsg(STransportRspMsg* msg) override;

  void DoClose(const CloseConnectionInfo& close_connection_info) override;

  void ThrottleConnection(uint64_t conn_id, bool set) override;

  bool AcceptConnection(AcceptConnectionInfo& connection_info);

  BindInfo& GetBindInfo() { return bind_info_; }

  const std::vector<BindAdapter*>& GetAdapters() const { return bind_adapters_; }

  void DecrAliveConnNum(int size) { alive_conns_.fetch_sub(size, std::memory_order_relaxed); }

 private:
  bool ListenUnix();
  bool ListenNet();
  bool ListenTcp();
  bool ListenUdp();
  bool ListenTcpUdp();

 private:
  std::vector<Reactor*> reactors_;

  std::vector<BindAdapter*> bind_adapters_;

  BindInfo bind_info_;

  std::atomic<int> alive_conns_{0};

  std::atomic<uint32_t> bind_adapters_index_{0};
};

}  // namespace trpc
