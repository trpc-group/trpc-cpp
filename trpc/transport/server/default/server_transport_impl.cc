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

#include "trpc/transport/server/default/server_transport_impl.h"

#include <algorithm>
#include <cstddef>
#include <memory>

#include "trpc/runtime/common/stats/frame_stats.h"
#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/runtime/iomodel/reactor/default/tcp_acceptor.h"
#include "trpc/runtime/iomodel/reactor/default/tcp_connection.h"
#include "trpc/transport/server/common/server_io_handler_factory.h"
#include "trpc/transport/server/default/bind_adapter.h"
#include "trpc/transport/server/default/server_connection_handler_factory.h"
#include "trpc/util/log/logging.h"

namespace trpc {

namespace {

inline int GetIoThreadIndex(uint64_t conn_id) { return ((0x0000FFFF00000000 & conn_id) >> 32); }

}  // namespace

ServerTransportImpl::ServerTransportImpl(std::vector<Reactor*>&& reactors) : reactors_(std::move(reactors)) {
  TRPC_ASSERT(reactors_.size() > 0);
}

ServerTransportImpl::~ServerTransportImpl() {
  for (auto* adapter : bind_adapters_) {
    delete adapter;
    adapter = nullptr;
  }
}

void ServerTransportImpl::Bind(const BindInfo& bind_info) {
#if !defined(SO_REUSEPORT) || defined(TRPC_DISABLE_REUSEPORT)
  // if not support reuseport, check accept_thread_num whether equal to 1
  TRPC_ASSERT(bind_info.accept_thread_num == 1);
#endif

  bind_info_ = bind_info;
  for (Reactor* reactor : reactors_) {
    BindAdapter::Options bind_adapter_option;
    bind_adapter_option.reactor = reactor;
    bind_adapter_option.transport = this;

    bind_adapters_.push_back(new BindAdapter(bind_adapter_option));
  }
}

bool ServerTransportImpl::Listen() {
  const std::string& socket_type = bind_info_.socket_type;
  bool listen_succ = false;
  if (socket_type == "unix" || socket_type == "local") {
    listen_succ = ListenUnix();
  } else {
    listen_succ = ListenNet();
  }
  if (!listen_succ) {
    return listen_succ;
  }

  if (bind_info_.network != "udp") {
    for (auto* adapter : bind_adapters_) {
      adapter->StartIdleConnCleanJob();
    }
  }

  return listen_succ;
}

bool ServerTransportImpl::ListenUnix() {
  uint32_t count = 0;
  uint32_t accept_num = std::min(uint32_t(bind_adapters_.size()), bind_info_.accept_thread_num);
  for (auto* adapter : bind_adapters_) {
    if (!adapter->Bind(trpc::BindAdapter::BindType::kUds)) {
      TRPC_LOG_ERROR("bind unix_path address:" << bind_info_.unix_path << " failed.");
      return false;
    }

    if (!adapter->Listen()) {
      TRPC_LOG_ERROR("listen unix_path address:" << bind_info_.unix_path << " failed.");
      return false;
    }

    if (++count >= accept_num) {
      break;
    }
  }

  return true;
}

bool ServerTransportImpl::ListenNet() {
  const std::string& network = bind_info_.network;
  if (network == "tcp,udp") {
    return ListenTcpUdp();
  } else if (network == "udp") {
    return ListenUdp();
  } else if (network == "tcp") {
    return ListenTcp();
  }

  // not support
  TRPC_FMT_ERROR("not support network: {}", network);

  return false;
}

bool ServerTransportImpl::ListenTcp() {
  uint32_t count = 0;
  uint32_t accept_num = std::min(uint32_t(bind_adapters_.size()), bind_info_.accept_thread_num);
  for (auto* adapter : bind_adapters_) {
    if (!adapter->Bind(trpc::BindAdapter::BindType::kTcp)) {
      TRPC_LOG_ERROR("bind tcp address: " << bind_info_.ip << ":" << bind_info_.port << " failed.");
      return false;
    }

    if (!adapter->Listen()) {
      TRPC_LOG_ERROR("listen tcp address: " << bind_info_.ip << ":" << bind_info_.port << " failed.");
      return false;
    }

    if (++count >= accept_num) {
      break;
    }
  }

  return true;
}

bool ServerTransportImpl::ListenUdp() {
  for (auto* adapter : bind_adapters_) {
    if (!adapter->Bind(trpc::BindAdapter::BindType::kUdp)) {
      TRPC_LOG_ERROR("bind udp address: " << bind_info_.ip << ":" << bind_info_.port << " failed.");
      return false;
    }

    if (!adapter->Listen()) {
      TRPC_LOG_ERROR("listen udp address: " << bind_info_.ip << ":" << bind_info_.port << " failed.");
      return false;
    }

#if !defined(SO_REUSEPORT) || defined(TRPC_DISABLE_REUSEPORT)
    break;
#endif
  }

  return true;
}

bool ServerTransportImpl::ListenTcpUdp() {
  uint32_t count = 0;
  uint32_t accept_num = std::min(uint32_t(bind_adapters_.size()), bind_info_.accept_thread_num);
  for (auto* adapter : bind_adapters_) {
    if (!adapter->Bind(trpc::BindAdapter::BindType::kTcp)) {
      TRPC_LOG_ERROR("network:tcp/udp, bind tcp address: " << bind_info_.ip << ":" << bind_info_.port << " failed.");
      return false;
    }

    if (++count >= accept_num) {
      break;
    }
  }

  for (auto* adapter : bind_adapters_) {
    if (!adapter->Bind(trpc::BindAdapter::BindType::kUdp)) {
      TRPC_LOG_ERROR("network:tcp/udp bind udp address: " << bind_info_.ip << ":" << bind_info_.port << " failed.");
      return false;
    }

#if !defined(SO_REUSEPORT) || defined(TRPC_DISABLE_REUSEPORT)
    break;
#endif
  }

  for (auto* adapter : bind_adapters_) {
    if (!adapter->Listen()) {
      TRPC_LOG_ERROR("network:tcp/udp, listen address: " << bind_info_.ip << ":" << bind_info_.port << " failed.");
      return false;
    }
  }

  return true;
}

void ServerTransportImpl::Stop() {
  for (auto& adapter : bind_adapters_) {
    adapter->Stop();
  }
}

void ServerTransportImpl::StopListen(bool clean_conn) {
  for (auto& adapter : bind_adapters_) {
    adapter->StopListen(clean_conn);
  }
}

void ServerTransportImpl::Destroy() {
  for (auto& adapter : bind_adapters_) {
    adapter->Destroy();
  }

  alive_conns_ = 0;
}

int ServerTransportImpl::SendMsg(STransportRspMsg* msg) {
  return bind_adapters_[GetIoThreadIndex(msg->context->GetConnectionId())]->SendMsg(msg);
}

bool ServerTransportImpl::AcceptConnection(AcceptConnectionInfo& connection_info) {
  int alive_conn_num = alive_conns_.fetch_add(1, std::memory_order_relaxed);
  if (alive_conn_num >= static_cast<int>(bind_info_.max_conn_num)) {
    TRPC_FMT_ERROR("too many connections {} and connection {} droped.",
        alive_conn_num, connection_info.conn_info.ToString());
    alive_conns_.fetch_sub(1, std::memory_order_relaxed);
    connection_info.socket.Close();
    return false;
  }

  if (TRPC_UNLIKELY(bind_info_.accept_function)) {
    if (bind_info_.accept_function(connection_info)) {
      TRPC_FMT_WARN("connection {} droped.", connection_info.conn_info.ToString());
      connection_info.socket.Close();
      return false;
    }
  }

  std::size_t index = bind_adapters_index_.fetch_add(1) % bind_adapters_.size();

  if (TRPC_UNLIKELY(bind_info_.dispatch_accept_function)) {
    index = bind_info_.dispatch_accept_function(connection_info, bind_adapters_.size());
  }
  BindAdapter* bind_adapter = bind_adapters_[index];
  Reactor* reactor = bind_adapter->GetOptions().reactor;
  bool ret = reactor->SubmitTask2([this, bind_adapter, connection_info]() mutable {
    uint64_t conn_id = bind_adapter->GetConnManager().GenConnectionId();
    if (conn_id == 0) {
      TRPC_FMT_ERROR("too many connections and connection {} droped.", connection_info.conn_info.ToString());
      connection_info.socket.Close();
      return;
    }

    TRPC_FMT_DEBUG("server accept connection {} and connid {}.", connection_info.conn_info.ToString(), conn_id);

    if (connection_info.conn_info.is_net) {
      connection_info.socket.SetTcpNoDelay();
    }

    connection_info.socket.SetCloseWaitDefault();
    connection_info.socket.SetBlock(false);
    connection_info.socket.SetKeepAlive();

    if (bind_info_.custom_set_socket_opt_function) {
      bind_info_.custom_set_socket_opt_function(connection_info.socket);
    }

    Reactor* reactor = bind_adapter->GetOptions().reactor;
    RefPtr<TcpConnection> conn = MakeRefCounted<TcpConnection>(reactor, connection_info.socket);
    conn->SetConnId(conn_id);
    conn->SetConnType(ConnectionType::kTcpLong);
    conn->SetMaxPacketSize(bind_info_.max_packet_size);
    conn->SetPeerIp(connection_info.conn_info.remote_addr.Ip());
    conn->SetPeerPort(connection_info.conn_info.remote_addr.Port());
    conn->SetPeerIpType(connection_info.conn_info.remote_addr.Type());
    conn->SetLocalIp(connection_info.conn_info.local_addr.Ip());
    conn->SetLocalPort(connection_info.conn_info.local_addr.Port());
    conn->SetLocalIpType(connection_info.conn_info.local_addr.Type());

    std::unique_ptr<IoHandler> io_handler =
        ServerIoHandlerFactory::GetInstance()->Create(bind_info_.protocol, conn.Get(), bind_info_);
    conn->SetIoHandler(std::move(io_handler));

    auto conn_handler =
        DefaultServerConnectionHandlerFactory::GetInstance()->Create(conn.Get(), bind_adapter, &bind_info_);
    conn_handler->Init();

    conn->SetConnectionHandler(std::move(conn_handler));

    bind_adapter->AddConnection(conn);

    conn->Established();
    conn->StartHandshaking();
  });

  if (!ret) {
    TRPC_FMT_ERROR("connection {} droped.", connection_info.conn_info.ToString());
    connection_info.socket.Close();
    return false;
  }

  FrameStats::GetInstance()->GetServerStats().AddConnCount(1);

  return true;
}

void ServerTransportImpl::DoClose(const CloseConnectionInfo& close_connection_info) {
  bind_adapters_[GetIoThreadIndex(close_connection_info.connection_id)]->DoClose(close_connection_info);
}

void ServerTransportImpl::ThrottleConnection(uint64_t conn_id, bool set) {
  bind_adapters_[GetIoThreadIndex(conn_id)]->ThrottleConnection(conn_id, set);
}

}  // namespace trpc
