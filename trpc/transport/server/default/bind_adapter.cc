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

#include "trpc/transport/server/default/bind_adapter.h"

#include <utility>
#include <vector>

#include "trpc/runtime/common/stats/frame_stats.h"
#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/runtime/iomodel/reactor/default/tcp_acceptor.h"
#include "trpc/runtime/iomodel/reactor/default/uds_acceptor.h"
#include "trpc/transport/server/common/server_io_handler_factory.h"
#include "trpc/transport/server/default/server_connection_handler_factory.h"
#include "trpc/util/latch.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc {

BindAdapter::BindAdapter(Options options)
    : options_(std::move(options)),
      conn_manager_(options_.reactor->Id(), options_.transport->GetBindInfo().max_conn_num) {
  TRPC_LOG_TRACE("BindAdapter reactor_id:" << options_.reactor->Id());
  TRPC_ASSERT(options_.reactor != nullptr);
  TRPC_ASSERT(options_.transport != nullptr);

  connection_idle_timeout_ = options_.transport->GetBindInfo().idle_time;

  active_conn_ids_.resize(0);

  idle_connections_.reserve(options_.transport->GetBindInfo().max_conn_num);
}

bool BindAdapter::Bind(BindType bind_type) {
  if (bind_type == BindType::kUds) {
    return BindUds();
  }

  if (bind_type == BindType::kTcp) {
    return BindTcp();
  }

  if (bind_type == BindType::kUdp) {
    return BindUdp();
  }

  return false;
}

bool BindAdapter::BindUds() {
  BindInfo& bind_info = options_.transport->GetBindInfo();
  UnixAddress addr(bind_info.unix_path.c_str());

  auto func = [this](AcceptConnectionInfo& connection_info) {
    return this->options_.transport->AcceptConnection(connection_info);
  };

  acceptor_ = MakeRefCounted<UdsAcceptor>(options_.reactor, addr);
  acceptor_->SetAcceptHandleFunction(std::move(func));
  if (bind_info.custom_set_accept_socket_opt_function) {
    acceptor_->SetAcceptSetSocketOptFunction(bind_info.custom_set_accept_socket_opt_function);
  }

  return true;
}

bool BindAdapter::BindTcp() {
  BindInfo& bind_info = options_.transport->GetBindInfo();
  NetworkAddress addr(bind_info.ip, bind_info.port,
                      bind_info.is_ipv6 ? NetworkAddress::IpType::kIpV6 : NetworkAddress::IpType::kIpV4);

  auto func = [this](AcceptConnectionInfo& connection_info) {
    return this->options_.transport->AcceptConnection(connection_info);
  };
  acceptor_ = MakeRefCounted<TcpAcceptor>(options_.reactor, addr);
  acceptor_->SetAcceptHandleFunction(std::move(func));
  if (bind_info.custom_set_accept_socket_opt_function) {
    acceptor_->SetAcceptSetSocketOptFunction(bind_info.custom_set_accept_socket_opt_function);
  }

  return true;
}

bool BindAdapter::BindUdp() {
  BindInfo& bind_info = options_.transport->GetBindInfo();
  NetworkAddress udp_addr(bind_info.ip, bind_info.port,
                          bind_info.is_ipv6 ? NetworkAddress::IpType::kIpV6 : NetworkAddress::IpType::kIpV4);

  Socket sock = Socket::CreateUdpSocket(udp_addr.IsIpv6());
  TRPC_ASSERT(sock.IsValid());
  sock.SetReuseAddr();
  sock.SetReusePort();
  sock.SetBlock(false);

  if (bind_info.custom_set_socket_opt_function) {
    bind_info.custom_set_socket_opt_function(sock);
  }

  if (!sock.Bind(udp_addr)) {
    return false;
  }

  uint64_t udp_transceiver_id = (0xFFFFFFFF00000000 & (static_cast<uint64_t>(options_.reactor->Id()) << 32));
  udp_transceiver_ = MakeRefCounted<UdpTransceiver>(options_.reactor, sock);
  udp_transceiver_->SetConnId(udp_transceiver_id);
  udp_transceiver_->SetIoHandler(ServerIoHandlerFactory::GetInstance()->Create(bind_info.protocol,
      udp_transceiver_.Get(), bind_info));

  auto conn_handler = DefaultServerConnectionHandlerFactory::GetInstance()->Create(
      udp_transceiver_.Get(), this, &bind_info);

  udp_transceiver_->SetConnectionHandler(std::move(conn_handler));
  udp_transceiver_->SetConnType(ConnectionType::kUdp);
  udp_transceiver_->SetMaxPacketSize(bind_info.max_packet_size);
  udp_transceiver_->SetLocalIp(udp_addr.Ip());
  udp_transceiver_->SetLocalPort(udp_addr.Port());
  udp_transceiver_->SetLocalIpType(udp_addr.Type());

  return true;
}

bool BindAdapter::Listen() {
  Latch l(1);

  bool result = true;
  Reactor::Task task = [this, &l, &result]() {
    if (this->acceptor_) {
      result = this->acceptor_->EnableListen();
      if (!result) {
        TRPC_LOG_ERROR("EnableListen failed.");
        l.count_down();
        return;
      }
    }

    if (udp_transceiver_) {
      udp_transceiver_->EnableReadWrite();
      udp_transceiver_->StartHandshaking();
    }

    l.count_down();
  };

  auto* reactor = options_.reactor;
  bool ret = reactor->SubmitTask2(std::move(task));
  if (ret) {
    if (reactor->GetCurrentTlsReactor() != reactor) {
      l.wait();
    }
  } else {
    TRPC_FMT_ERROR("Listen failed.");
    result = false;
  }

  return result;
}

void BindAdapter::StopListen(bool clean_conn) {
  Reactor::Task task = [this, clean_conn] {
    if (this->acceptor_) {
      this->acceptor_->DisableListen();

      this->acceptor_.Reset();
    }

    if (clean_conn) {
      conn_manager_.DisableAllConnRead();
    }

    TRPC_LOG_DEBUG("BindAdapter::StopListen Stop listen done, clean_conn:" << clean_conn);
  };

  bool ret = options_.reactor->SubmitTask(std::move(task));
  if (!ret) {
    TRPC_FMT_ERROR("StopListen clean_conn: {} failed.", clean_conn);
  }
}

void BindAdapter::StartIdleConnCleanJob() {
  if (connection_idle_timeout_ > 0) {
    Latch l(1);
    bool ret = options_.reactor->SubmitTask2([this, &l]() {
      timer_id_ = iotimer::Create(0, 1000, [this]() { this->RemoveIdleConnection(); });
      l.count_down();
    });

    if (ret) {
      if (options_.reactor->GetCurrentTlsReactor() != options_.reactor) {
        l.wait();
      }
    } else {
      TRPC_FMT_ERROR("StartIdleConnCleanJob failed.");
    }
  }
}

void BindAdapter::Stop() {
  Latch l(1);

  Reactor::Task task = [this, &l]() {
    if (this->timer_id_ != kInvalidTimerId) {
      iotimer::Cancel(timer_id_);
      timer_id_ = -1;
    }

    if (this->acceptor_) {
      acceptor_->DisableListen();
    }

    conn_manager_.Stop();

    if (udp_transceiver_) {
      udp_transceiver_->DisableReadWrite();
    }

    l.count_down();
  };

  auto* reactor = options_.reactor;
  bool ret = reactor->SubmitTask2(std::move(task));
  if (ret) {
    if (reactor->GetCurrentTlsReactor() != reactor) {
      l.wait();
    }
  } else {
    TRPC_FMT_ERROR("Stop failed.");
  }
}

void BindAdapter::Destroy() {
  conn_manager_.Destroy();
}

void BindAdapter::ThrottleConnection(uint64_t conn_id, bool set) {
  Reactor::Task task = [this, conn_id, set]() {
    TcpConnection* conn = conn_manager_.GetConnection(conn_id);
    if (TRPC_UNLIKELY(conn == nullptr)) {
      TRPC_LOG_ERROR("BindAdapter::ThrottleConnection conn_id:" << conn_id << ", set:" << set
                                                                << ", connection is not exist.");
      return;
    }

    conn->Throttle(set);
  };

  bool ret = options_.reactor->SubmitTask2(std::move(task));
  if (!ret) {
    TRPC_LOG_ERROR("BindAdapter::ThrottleConnection conn_id:" << conn_id << ", set:" << set
                                                                << ", submit task failed.");
  }
}

void BindAdapter::AddConnection(const RefPtr<TcpConnection>& conn) {
  std::list<uint64_t>::iterator it = active_conn_ids_.insert(active_conn_ids_.end(), conn->GetConnId());
  conn->SetContext(it);

  TRPC_ASSERT(conn_manager_.AddConnection(conn));
}

void BindAdapter::UpdateConnection(Connection* conn) {
  std::list<uint64_t>::iterator old_it = conn->GetContext();

  active_conn_ids_.erase(old_it);

  std::list<uint64_t>::iterator new_it = active_conn_ids_.insert(active_conn_ids_.end(), conn->GetConnId());
  conn->SetContext(new_it);
}

void BindAdapter::DelConnection(TcpConnection* conn) {
  TRPC_LOG_TRACE("DelConnection conn_id:" << conn->GetConnId());

  conn->DoClose(true);
}

void BindAdapter::CleanConnectionResource(Connection* conn) {
  TRPC_LOG_TRACE("CleanConnectionResource conn_id:" << conn->GetConnId());

  std::list<uint64_t>::iterator old_it = conn->GetContext();
  active_conn_ids_.erase(old_it);

  RefPtr<TcpConnection> conn_ptr = conn_manager_.DelConnection(conn->GetConnId());

  options_.transport->DecrAliveConnNum(1);

  FrameStats::GetInstance()->GetServerStats().AddConnCount(-1);

  conn->GetConnectionHandler()->Stop();
  conn->GetConnectionHandler()->Join();

  options_.reactor->SubmitInnerTask([temp = std::move(conn_ptr)] {});
}

void BindAdapter::RemoveIdleConnection() {
  if (connection_idle_timeout_ == 0) {
    return;
  }

  TRPC_LOG_TRACE("RemoveIdleConnection connection_idle_timeout_: " << connection_idle_timeout_);
  idle_connections_.clear();

  uint64_t now = trpc::time::GetMilliSeconds();
  for (auto& item : active_conn_ids_) {
    TcpConnection* conn = conn_manager_.GetConnection(item);
    if (conn) {
      if (now - conn->GetConnActiveTime() > connection_idle_timeout_) {
        idle_connections_.push_back(item);
      } else {
        break;
      }
    }
  }

  if (idle_connections_.empty()) {
    return;
  }

  TRPC_LOG_TRACE("RemoveIdleConnection remove_connections num: " << idle_connections_.size());

  options_.reactor->SubmitInnerTask([this] {
    for (auto conn_id : this->idle_connections_) {
      TcpConnection* conn = conn_manager_.GetConnection(conn_id);
      if (conn) {
        DelConnection(conn);
      }
    }
  });
}

int BindAdapter::SendMsg(STransportRspMsg* msg) {
  BindInfo& bind_info = options_.transport->GetBindInfo();
  if (bind_info.run_server_filters_function(FilterPoint::SERVER_PRE_SCHED_SEND_MSG, msg) == FilterStatus::REJECT) {
    bind_info.run_server_filters_function(FilterPoint::SERVER_POST_SCHED_SEND_MSG, msg);
    object_pool::Delete<STransportRspMsg>(msg);
    return -1;
  }

  bool ret = options_.reactor->SubmitTask2([this, msg]() {
    BindInfo& bind_info = this->options_.transport->GetBindInfo();
    bind_info.run_server_filters_function(FilterPoint::SERVER_POST_SCHED_SEND_MSG, msg);

    if (msg->context->GetNetType() == ServerContext::NetType::kTcp) {
      SendTcpMsg(msg);
    } else if (msg->context->GetNetType() == ServerContext::NetType::kUdp) {
      SendUdpMsg(msg);
    }

    object_pool::Delete<STransportRspMsg>(msg);
  });

  if (!ret) {
    bind_info.run_server_filters_function(FilterPoint::SERVER_POST_SCHED_SEND_MSG, msg);
    object_pool::Delete<STransportRspMsg>(msg);
    return -1;
  }

  return 0;
}

int BindAdapter::SendTcpMsg(STransportRspMsg* msg) {
  int ret = -1;
  auto& context = msg->context;
  TcpConnection* conn = conn_manager_.GetConnection(context->GetConnectionId());

  if (conn) {
    uint32_t stream_id = context->GetStreamId();

    IoMessage message;
    message.msg = std::move(msg->context);
    message.buffer = std::move(msg->buffer);

    if (stream_id == 0) {
      ret = conn->Send(std::move(message));
    } else {
      if (TRPC_LIKELY(conn->GetConnectionHandler()->EncodeStreamMessage(&message))) {
        ret = conn->Send(std::move(message));
      }
    }
  }
  return ret;
}

int BindAdapter::SendUdpMsg(STransportRspMsg* msg) {
  IoMessage message;
  message.ip = msg->context->GetIp();
  message.port = msg->context->GetPort();
  message.msg = std::move(msg->context);
  message.buffer = std::move(msg->buffer);

  return udp_transceiver_->Send(std::move(message));
}

void BindAdapter::DoClose(const CloseConnectionInfo& close_connection_info) {
  bool ret = options_.reactor->SubmitTask([this, close_connection_info]() {
    TcpConnection* conn = conn_manager_.GetConnection(close_connection_info.connection_id);
    if (conn == nullptr) {
      TRPC_LOG_DEBUG("fail to find connectionptr for conn_id:" << close_connection_info.connection_id <<
                     ", client_ip:" << close_connection_info.client_ip <<
                     ", client_port:" << close_connection_info.client_port <<
                     ", fd:" << close_connection_info.fd);
      return;
    }
    if (conn->GetPeerIp() == close_connection_info.client_ip &&
        conn->GetPeerPort() == close_connection_info.client_port && conn->GetFd() == close_connection_info.fd) {
      DelConnection(conn);
      TRPC_LOG_DEBUG("DoClose conn_id: " << conn->GetConnId());
    } else {
      TRPC_LOG_ERROR("Connection is not valid, connectionId:" << close_connection_info.connection_id <<
                     ", conn.ip:" << conn->GetPeerIp() << ", info.ip:" << close_connection_info.client_ip <<
                     ", conn.port:" << conn->GetPeerPort() << ", info.port:" << close_connection_info.client_port <<
                     ", conn.fd:" << conn->GetFd() << ", info.fd:" << close_connection_info.fd);
    }
  });

  if (!ret) {
    TRPC_LOG_ERROR("submitTask failed, connectionId:" << close_connection_info.connection_id <<
                   ", info.ip:" << close_connection_info.client_ip <<
                   ", info.port:" << close_connection_info.client_port <<
                   ", info.fd:" << close_connection_info.fd);
  }

  return;
}

}  // namespace trpc
