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

#pragma once

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "trpc/coroutine/fiber_timer.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_acceptor.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_tcp_connection.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_udp_transceiver.h"
#include "trpc/transport/server/fiber/fiber_connection_manager.h"
#include "trpc/transport/server/server_transport_message.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

class FiberServerTransportImpl;

/// @brief Bind adapter for fiber tcp/udp
/// @note  Each fiber scheduling has its owned instance
class FiberBindAdapter : public RefCounted<FiberBindAdapter> {
 public:
  explicit FiberBindAdapter(FiberServerTransportImpl* transport, size_t scheduling_group_index);

  bool Bind();

  bool Listen();

  void StopListen(bool clean_conn);

  void Stop();

  void Destroy();

  int SendMsg(STransportRspMsg* msg);

  uint64_t GenConnectionId();

  void AddConnection(RefPtr<FiberTcpConnection>&& conn);

  void UpdateConnection(Connection* conn) {}

  void CleanConnectionResource(Connection* conn);

  bool IsConnected(uint64_t connection_id);

  void DoClose(const CloseConnectionInfo& close_connection_info);

  FiberServerTransportImpl* GetTransport() { return transport_; }

 private:
  RefPtr<FiberTcpConnection> GetConnection(uint64_t conn_id);
  bool BindTcp();
  bool BindUdp();
  void RemoveIdleConnection();
  int SendTcpMsg(STransportRspMsg* msg);
  int SendUdpMsg(STransportRspMsg* msg);

 private:
  FiberServerTransportImpl* transport_;

  size_t scheduling_group_index_;

  uint64_t idle_conn_cleaner_{0};

  uint32_t connection_idle_timeout_{0};

  uint32_t max_conn_num_{100000};

  std::vector<RefPtr<FiberAcceptor>> acceptors_;

  std::vector<RefPtr<FiberUdpTransceiver>> udp_transceivers_;

  FiberConnectionManager connection_manager_;
};

}  // namespace trpc
