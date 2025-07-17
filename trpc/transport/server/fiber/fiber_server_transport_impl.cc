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

#include "trpc/transport/server/fiber/fiber_server_transport_impl.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>

#include "trpc/runtime/common/stats/frame_stats.h"
#include "trpc/runtime/fiber_runtime.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_acceptor.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_reactor.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_tcp_connection.h"
#include "trpc/transport/server/common/server_io_handler_factory.h"
#include "trpc/transport/server/fiber/fiber_server_connection_handler_factory.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/random.h"

namespace trpc {

namespace {

inline int GetIndexOfBindAdapter(uint64_t conn_id) { return ((0xFFFFFFFF00000000 & conn_id) >> 32); }

}  // namespace

FiberServerTransportImpl::~FiberServerTransportImpl() { bind_adapters_.clear(); }

void FiberServerTransportImpl::Bind(const BindInfo& bind_info) {
  bind_info_ = bind_info;
  size_t scheduling_group_count = fiber::GetSchedulingGroupCount();
  for (size_t i = 0; i < scheduling_group_count; ++i) {
    bind_adapters_.emplace_back(MakeRefCounted<FiberBindAdapter>(this, i));
  }
}

bool FiberServerTransportImpl::Listen() {
  TRPC_ASSERT(!bind_adapters_.empty());
  for (auto& adapter : bind_adapters_) {
    if (!adapter->Bind()) {
      return false;
    }

    if (!adapter->Listen()) {
      return false;
    }
#if !defined(SO_REUSEPORT) || defined(TRPC_DISABLE_REUSEPORT)
    break;
#endif
  }

  return true;
}

void FiberServerTransportImpl::Stop() {
  for (auto& adapter : bind_adapters_) {
    adapter->Stop();
  }

  alive_conns_ = 0;
}

void FiberServerTransportImpl::StopListen(bool clean_conn) {
  for (auto& adapter : bind_adapters_) {
    adapter->StopListen(clean_conn);
  }
}

void FiberServerTransportImpl::Destroy() {
  for (auto& adapter : bind_adapters_) {
    adapter->Destroy();
  }
}

int FiberServerTransportImpl::SendMsg(STransportRspMsg* msg) {
  return bind_adapters_[GetIndexOfBindAdapter(msg->context->GetConnectionId())]->SendMsg(msg);
}

bool FiberServerTransportImpl::AcceptConnection(AcceptConnectionInfo& connection_info) {
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

#if defined(SO_REUSEPORT) && !defined(TRPC_DISABLE_REUSEPORT)
  std::size_t scheduling_group_index = fiber::GetCurrentSchedulingGroupIndex();
#else
  std::size_t scheduling_group_index = Random() % fiber::GetSchedulingGroupCount();
#endif

  if (TRPC_UNLIKELY(bind_info_.dispatch_accept_function)) {
    scheduling_group_index = bind_info_.dispatch_accept_function(connection_info, fiber::GetSchedulingGroupCount());
  }

  uint64_t conn_id = bind_adapters_[scheduling_group_index]->GenConnectionId();
  if (conn_id == 0) {
    TRPC_FMT_ERROR("too many connections and connection {} droped.", connection_info.conn_info.ToString());
    connection_info.socket.Close();
    return false;
  }

  Reactor* reactor = fiber::GetReactor(scheduling_group_index, connection_info.socket.GetFd());
  TRPC_ASSERT(reactor != nullptr);

  TRPC_FMT_DEBUG("server accept connection {} and connid {}.", connection_info.conn_info.ToString(), conn_id);

  connection_info.socket.SetTcpNoDelay();
  connection_info.socket.SetCloseWaitDefault();
  connection_info.socket.SetBlock(false);
  connection_info.socket.SetKeepAlive();

  if (bind_info_.custom_set_socket_opt_function) {
    bind_info_.custom_set_socket_opt_function(connection_info.socket);
  }

  auto conn = MakeRefCounted<FiberTcpConnection>(reactor, connection_info.socket);
  conn->SetConnId(conn_id);
  std::unique_ptr<IoHandler> io_handler =
      ServerIoHandlerFactory::GetInstance()->Create(bind_info_.protocol, conn.Get(), bind_info_);
  conn->SetIoHandler(std::move(io_handler));
  conn->SetConnType(ConnectionType::kTcpLong);
  conn->SetMaxPacketSize(bind_info_.max_packet_size);
  conn->SetRecvBufferSize(bind_info_.recv_buffer_size);
  conn->SetSendQueueCapacity(bind_info_.send_queue_capacity);
  conn->SetSendQueueTimeout(bind_info_.send_queue_timeout);
  conn->SetPeerIp(connection_info.conn_info.remote_addr.Ip());
  conn->SetPeerPort(connection_info.conn_info.remote_addr.Port());
  conn->SetPeerIpType(connection_info.conn_info.remote_addr.Type());
  conn->SetLocalIp(connection_info.conn_info.local_addr.Ip());
  conn->SetLocalPort(connection_info.conn_info.local_addr.Port());
  conn->SetLocalIpType(connection_info.conn_info.local_addr.Type());

  auto conn_handler = FiberServerConnectionHandlerFactory::GetInstance()->Create(
      conn.Get(), bind_adapters_[scheduling_group_index].Get(), &bind_info_);
  conn_handler->Init();

  conn->SetConnectionHandler(std::move(conn_handler));

  conn->Established();
  conn->StartHandshaking();

  bind_adapters_[scheduling_group_index]->AddConnection(std::move(conn));

  FrameStats::GetInstance()->GetServerStats().AddConnCount(1);

  return true;
}

bool FiberServerTransportImpl::IsConnected(uint64_t connection_id) {
  return bind_adapters_[GetIndexOfBindAdapter(connection_id)]->IsConnected(connection_id);
}

void FiberServerTransportImpl::DoClose(const CloseConnectionInfo& close_connection_info) {
  bind_adapters_[GetIndexOfBindAdapter(close_connection_info.connection_id)]->DoClose(close_connection_info);
}

}  // namespace trpc
