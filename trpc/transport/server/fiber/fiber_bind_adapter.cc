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

#include "trpc/transport/server/fiber/fiber_bind_adapter.h"

#include "trpc/coroutine/fiber.h"
#include "trpc/runtime/common/stats/frame_stats.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_reactor.h"
#include "trpc/transport/server/common/server_io_handler_factory.h"
#include "trpc/transport/server/fiber/fiber_server_connection_handler_factory.h"
#include "trpc/transport/server/fiber/fiber_server_transport_impl.h"
#include "trpc/util/time.h"

using namespace std::literals;

namespace trpc {

FiberBindAdapter::FiberBindAdapter(FiberServerTransportImpl* transport, size_t scheduling_group_index)
    : transport_(transport), scheduling_group_index_(scheduling_group_index) {
  TRPC_ASSERT(transport_ != nullptr);

  const BindInfo& bind_info = transport_->GetBindInfo();

  connection_idle_timeout_ = bind_info.idle_time;
  max_conn_num_ = bind_info.max_conn_num;
}

bool FiberBindAdapter::Bind() {
  const BindInfo& bind_info = transport_->GetBindInfo();
  if (bind_info.network == "tcp") {
    return BindTcp();
  }

  if (bind_info.network == "udp") {
    connection_idle_timeout_ = 0;
    return BindUdp();
  }

  if (bind_info.network == "tcp,udp") {
    if (!BindTcp()) {
      return false;
    }

    return BindUdp();
  }

  TRPC_LOG_ERROR("only support 'tcp', 'udp', 'tcp,udp'");
  TRPC_ASSERT((bind_info.network == "tcp" || bind_info.network == "udp" || bind_info.network == "tcp,udp") &&
              "only support 'tcp', 'udp', 'tcp,udp'");
  return false;
}

bool FiberBindAdapter::BindTcp() {
  const BindInfo& bind_info = transport_->GetBindInfo();
  NetworkAddress addr(bind_info.ip, bind_info.port,
                      bind_info.is_ipv6 ? NetworkAddress::IpType::kIpV6 : NetworkAddress::IpType::kIpV4);
  TRPC_LOG_DEBUG("make acceptor addr:" << addr.ToString());

  std::vector<Reactor*> reactors;
  fiber::GetReactorInSameGroup(scheduling_group_index_, reactors);

  TRPC_ASSERT(!reactors.empty());

  uint32_t accept_num = std::min(uint32_t(reactors.size()), bind_info.accept_thread_num);
  TRPC_ASSERT(accept_num > 0);

#if !defined(SO_REUSEPORT) || defined(TRPC_DISABLE_REUSEPORT)
  TRPC_ASSERT(accept_num == 1);
#endif

  uint32_t count = 0;
  for (auto* reactor : reactors) {
    auto func = [this](AcceptConnectionInfo& connection_info) {
      return this->transport_->AcceptConnection(connection_info);
    };
    auto acceptor = MakeRefCounted<FiberAcceptor>(reactor, addr);
    acceptor->SetAcceptHandleFunction(std::move(func));
    if (bind_info.custom_set_accept_socket_opt_function) {
      acceptor->SetAcceptSetSocketOptFunction(bind_info.custom_set_accept_socket_opt_function);
    }

    acceptors_.emplace_back(std::move(acceptor));

    if (++count >= accept_num) {
      break;
    }
  }

  return true;
}

bool FiberBindAdapter::BindUdp() {
  BindInfo& bind_info = transport_->GetBindInfo();
  NetworkAddress udp_addr(bind_info.ip, bind_info.port,
                          bind_info.is_ipv6 ? NetworkAddress::IpType::kIpV6 : NetworkAddress::IpType::kIpV4);

  std::vector<Reactor*> reactors;
  fiber::GetReactorInSameGroup(scheduling_group_index_, reactors);

  TRPC_ASSERT(!reactors.empty());

  uint64_t udp_transceiver_id = 0;
  for (auto* reactor : reactors) {
    Socket socket = Socket::CreateUdpSocket(bind_info.is_ipv6);
    TRPC_ASSERT(socket.IsValid());
    socket.SetReuseAddr();
    socket.SetReusePort();
    socket.SetBlock(false);

    if (bind_info.custom_set_socket_opt_function) {
      bind_info.custom_set_socket_opt_function(socket);
    }

    if (!socket.Bind(udp_addr)) {
      TRPC_LOG_ERROR("Udp Bind fail,address:" << udp_addr.ToString());
      return false;
    }

    auto udp_transceiver = MakeRefCounted<FiberUdpTransceiver>(reactor, socket);
    uint64_t id = (0xFFFFFFFF00000000 & (static_cast<uint64_t>(scheduling_group_index_) << 32));
    id = id | static_cast<uint64_t>(udp_transceiver_id);
    udp_transceiver->SetConnId(id);
    udp_transceiver->SetIoHandler(ServerIoHandlerFactory::GetInstance()->Create(bind_info.protocol,
        udp_transceiver.Get(), bind_info));

    auto conn_handler = FiberServerConnectionHandlerFactory::GetInstance()->Create(
      udp_transceiver.Get(), this, &bind_info);

    udp_transceiver->SetConnectionHandler(std::move(conn_handler));
    udp_transceiver->SetConnType(ConnectionType::kUdp);
    udp_transceiver->SetMaxPacketSize(bind_info.max_packet_size);
    std::string local_ip = bind_info.ip;
    udp_transceiver->SetLocalIp(std::move(local_ip));
    udp_transceiver->SetLocalPort(bind_info.port);
    udp_transceiver->SetLocalIpType(bind_info.is_ipv6 ? NetworkAddress::IpType::kIpV6 : NetworkAddress::IpType::kIpV4);

    udp_transceivers_.emplace_back(std::move(udp_transceiver));
    ++udp_transceiver_id;

#if !defined(SO_REUSEPORT) || defined(TRPC_DISABLE_REUSEPORT)
    break;
#endif
  }

  return true;
}

bool FiberBindAdapter::Listen() {
  for (auto& acceptor : acceptors_) {
    if (!acceptor->Listen()) {
      TRPC_LOG_ERROR("FiberAcceptor Listen fail");
      return false;
    }
    if (connection_idle_timeout_ > 0) {
        idle_conn_cleaner_ = SetFiberTimer(ReadSteadyClock(), std::chrono::seconds(1),
                                           [this, ref = RefPtr(ref_ptr, this)] { RemoveIdleConnection(); });
    }
  }

  for (auto& udp_transceiver : udp_transceivers_) {
    udp_transceiver->EnableReadWrite();
  }

  return true;
}

void FiberBindAdapter::Stop() {
  if (connection_idle_timeout_ > 0 && idle_conn_cleaner_) {
    KillFiberTimer(idle_conn_cleaner_);
  }

  for (const auto& acceptor : acceptors_) {
    acceptor->Stop();
    acceptor->Join();
  }

  for (const auto& udp_transceiver : udp_transceivers_) {
    udp_transceiver->Stop();
    udp_transceiver->Join();
  }

  connection_manager_.Stop();
}
void FiberBindAdapter::Destroy() { connection_manager_.Destroy(); }

void FiberBindAdapter::StopListen(bool clean_conn) {
  for (const auto& acceptor : acceptors_) {
    acceptor->Stop();
    acceptor->Join();

    if (clean_conn) {
      connection_manager_.DisableAllConnRead();
    }
  }
  acceptors_.clear();
}

int FiberBindAdapter::SendMsg(STransportRspMsg* msg) {
  BindInfo& bind_info = transport_->GetBindInfo();
  if (bind_info.run_server_filters_function(FilterPoint::SERVER_PRE_SCHED_SEND_MSG, msg) == FilterStatus::REJECT) {
    bind_info.run_server_filters_function(FilterPoint::SERVER_POST_SCHED_SEND_MSG, msg);
    object_pool::Delete<STransportRspMsg>(msg);
    return -1;
  }

  bind_info.run_server_filters_function(FilterPoint::SERVER_POST_SCHED_SEND_MSG, msg);

  int ret = -1;
  if (msg->context->GetNetType() == ServerContext::NetType::kTcp) {
    ret = SendTcpMsg(msg);
  } else if (msg->context->GetNetType() == ServerContext::NetType::kUdp) {
    ret = SendUdpMsg(msg);
  }

  object_pool::Delete<STransportRspMsg>(msg);

  return ret;
}

int FiberBindAdapter::SendTcpMsg(STransportRspMsg* msg) {
  auto& context = msg->context;
  uint32_t stream_id = context->GetStreamId();

  // If the return packet call of the request is under the same fiber
  // as the processing of the request, directly send the response
  if (context->GetReserved() != nullptr) {
    auto* fiber_conn = static_cast<FiberTcpConnection*>(context->GetReserved());

    IoMessage message;
    message.msg = std::move(msg->context);
    message.buffer = std::move(msg->buffer);

    int ret = -1;
    if (stream_id == 0) {
      ret = fiber_conn->Send(std::move(message));
    } else if (TRPC_LIKELY(fiber_conn->GetConnectionHandler()->EncodeStreamMessage(&message))) {
      ret = fiber_conn->Send(std::move(message));
    }
    return ret;
  }

  int ret = -1;
  auto fiber_conn = GetConnection(context->GetConnectionId());
  if (fiber_conn) {
    IoMessage message;
    message.msg = std::move(msg->context);
    message.buffer = std::move(msg->buffer);

    if (stream_id == 0 ||
        TRPC_LIKELY(fiber_conn->GetConnectionHandler()->EncodeStreamMessage(&message))) {
      ret = fiber_conn->Send(std::move(message));
    }
  }

  return ret;
}

int FiberBindAdapter::SendUdpMsg(STransportRspMsg* msg) {
  auto& context = msg->context;

  // If the return packet call of the request is under the same fiber
  // as the processing of the request, directly send the response
  if (context->GetReserved() != nullptr) {
    auto* fiber_conn = static_cast<FiberUdpTransceiver*>(context->GetReserved());

    IoMessage message;
    message.ip = context->GetIp();
    message.port = context->GetPort();
    message.msg = std::move(msg->context);
    message.buffer = std::move(msg->buffer);

    return fiber_conn->Send(std::move(message));
  }

  uint32_t connection_id = context->GetConnectionId();

  IoMessage message;
  message.ip = context->GetIp();
  message.port = context->GetPort();
  message.msg = std::move(msg->context);
  message.buffer = std::move(msg->buffer);

#if defined(SO_REUSEPORT) && !defined(TRPC_DISABLE_REUSEPORT)
  uint32_t index = (0x00000000FFFFFFFF & connection_id);
  return udp_transceivers_[index]->Send(std::move(message));
#else
  return udp_transceivers_[0]->Send(std::move(message));
#endif
}

uint64_t FiberBindAdapter::GenConnectionId() {
  static std::atomic<uint32_t> conn_id = 0;

  return (static_cast<uint64_t>(scheduling_group_index_) << 32) | ++conn_id;
}

void FiberBindAdapter::AddConnection(RefPtr<FiberTcpConnection>&& conn) {
  connection_manager_.Add(conn->GetConnId(), std::move(conn));
}

RefPtr<FiberTcpConnection> FiberBindAdapter::GetConnection(uint64_t conn_id) {
  return connection_manager_.Get(conn_id);
}

void FiberBindAdapter::CleanConnectionResource(Connection* conn) {
  uint64_t conn_id = conn->GetConnId();

  TRPC_LOG_DEBUG("CleanConnection conn_id:" << conn_id);

  RefPtr<FiberTcpConnection> fiber_conn = connection_manager_.Del(conn_id);
  if (!fiber_conn) {
    TRPC_LOG_DEBUG("ConnectionID:" << conn_id
                                   << " is not found , Perhaps it's removed by `RemoveIdleConnection()` Or `DoClose`");
    return;
  }

  bool start_fiber = StartFiberDetached([this, conn = std::move(fiber_conn)] {
    TRPC_LOG_DEBUG("fiber tcp conn_id:" << conn->GetConnId() << ", conn unsafe refcount:" << conn->UnsafeRefCount());
    conn->GetConnectionHandler()->Stop();
    conn->GetConnectionHandler()->Join();
    conn->Stop();
    conn->Join();

    this->transport_->DecrAliveConnNum(1);

    FrameStats::GetInstance()->GetServerStats().AddConnCount(-1);
  });

  TRPC_ASSERT(start_fiber && "StartFiberDetached failed when CleanConnectionResource");
}

void FiberBindAdapter::RemoveIdleConnection() {
  if (connection_idle_timeout_ == 0) {
    return;
  }

  std::vector<RefPtr<FiberTcpConnection>> idle_connections;
  connection_manager_.GetIdles(connection_idle_timeout_, idle_connections);

  if (idle_connections.empty()) {
    return;
  }

  TRPC_LOG_DEBUG("need clean connection size:" << idle_connections.size());

  bool start_fiber = StartFiberDetached([this, idle_connections = std::move(idle_connections)]() mutable {
    for (auto&& e : idle_connections) {
      e->GetConnectionHandler()->Stop();
    }
    for (auto&& e : idle_connections) {
      e->GetConnectionHandler()->Join();
    }
    for (auto&& e : idle_connections) {
      TRPC_LOG_DEBUG("RemoveIdleConnection Stop conn_id:" << e->GetConnId());
      e->Stop();
    }
    for (auto&& e : idle_connections) {
      e->Join();
    }

    this->transport_->DecrAliveConnNum(idle_connections.size());

    FrameStats::GetInstance()->GetServerStats().AddConnCount(-1 * idle_connections.size());
  });

  TRPC_ASSERT(start_fiber && "StartFiberDetached failed when RemoveIdleConnection");
}

bool FiberBindAdapter::IsConnected(uint64_t connection_id) { return GetConnection(connection_id) != nullptr; }

void FiberBindAdapter::DoClose(const CloseConnectionInfo& close_connection_info) {
  auto fiber_conn = connection_manager_.Del(close_connection_info.connection_id);

  if (!fiber_conn) {
    TRPC_LOG_DEBUG("Fail to find connectionptr for conn_id:" << close_connection_info.connection_id <<
                   ", client_ip:" << close_connection_info.client_ip <<
                   ", client_port:" << close_connection_info.client_port <<
                   ", fd:" << close_connection_info.fd);
    return;
  }

  if (fiber_conn->GetPeerIp() == close_connection_info.client_ip &&
      fiber_conn->GetPeerPort() == close_connection_info.client_port &&
      fiber_conn->GetFd() == close_connection_info.fd) {
    bool start_fiber = StartFiberDetached([this, fiber_conn]() {
      fiber_conn->GetConnectionHandler()->Stop();
      fiber_conn->GetConnectionHandler()->Join();
      fiber_conn->Stop();
      fiber_conn->Join();

      this->transport_->DecrAliveConnNum(1);
      FrameStats::GetInstance()->GetServerStats().AddConnCount(-1);

      TRPC_LOG_DEBUG("DoClose conn_id: " << fiber_conn->GetConnId());
    });

    TRPC_ASSERT(start_fiber && "StartFiberDetached failed when DoClose");
  } else {
    TRPC_LOG_ERROR("Connection is not valid. connectionId:" << close_connection_info.connection_id <<
                   ", conn.ip:" << fiber_conn->GetPeerIp() << ", info.ip:" << close_connection_info.client_ip <<
                   ", conn.port:" << fiber_conn->GetPeerPort() << ", info.port:" << close_connection_info.client_port <<
                   ", conn.fd:" << fiber_conn->GetFd() << ", info.fd:" << close_connection_info.fd);
    connection_manager_.Add(fiber_conn->GetConnId(), std::move(fiber_conn));
  }
}

}  // namespace trpc
