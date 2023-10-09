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
#include <unordered_map>
#include <utility>

#include "trpc/common/async_timer.h"
#include "trpc/runtime/iomodel/reactor/default/acceptor.h"
#include "trpc/runtime/iomodel/reactor/default/udp_transceiver.h"
#include "trpc/runtime/iomodel/reactor/reactor.h"
#include "trpc/transport/server/default/connection_manager.h"
#include "trpc/transport/server/default/server_transport_impl.h"
#include "trpc/util/time.h"

namespace trpc {

/// @brief Bind adapter for tcp/udp/uds
/// @note  Each io thread has its owned instance
class BindAdapter {
 public:
  enum class BindType : uint8_t {
    kTcp,
    kUdp,
    kUds,
  };

  struct Options {
    /// not owned
    Reactor* reactor;
    /// not owned
    ServerTransportImpl* transport;
  };

  explicit BindAdapter(Options options);

  ~BindAdapter() = default;

  bool Bind(BindType bind_type);

  bool Listen();

  void StartIdleConnCleanJob();

  void StopListen(bool clean_conn);

  void Stop();

  void Destroy();

  int SendMsg(STransportRspMsg* msg);

  void DoClose(const CloseConnectionInfo& close_connection_info);

  void AddConnection(const RefPtr<TcpConnection>& conn);

  void UpdateConnection(Connection* conn);

  void ThrottleConnection(uint64_t conn_id, bool set);

  void CleanConnectionResource(Connection* conn);

  ConnectionManager& GetConnManager() { return conn_manager_; }

  ServerTransportImpl* GetTransport() { return options_.transport; }

  BindAdapter::Options& GetOptions() { return options_; }

 private:
  bool BindUds();
  bool BindTcp();
  bool BindUdp();
  void DelConnection(TcpConnection* conn);
  void RemoveIdleConnection();
  int SendTcpMsg(STransportRspMsg* msg);
  int SendUdpMsg(STransportRspMsg* msg);

 private:
  Options options_;

  RefPtr<Acceptor> acceptor_{nullptr};

  RefPtr<UdpTransceiver> udp_transceiver_{nullptr};

  uint64_t connection_idle_timeout_{0};

  uint64_t timer_id_{kInvalidTimerId};

  ConnectionManager conn_manager_;

  std::list<uint64_t> active_conn_ids_;

  std::vector<uint64_t> idle_connections_;
};

}  // namespace trpc
