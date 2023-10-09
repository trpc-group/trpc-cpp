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

#include "trpc/transport/client/fiber/fiber_connector_group_manager.h"

#include <string>
#include <string_view>
#include <utility>

#include "trpc/transport/client/fiber/conn_complex/fiber_tcp_conn_complex_connector_group.h"
#include "trpc/transport/client/fiber/conn_complex/fiber_udp_io_complex_connector_group.h"
#include "trpc/transport/client/fiber/conn_pool/fiber_tcp_conn_pool_connector_group.h"
#include "trpc/transport/client/fiber/conn_pool/fiber_udp_io_pool_connector_group.h"
#include "trpc/transport/client/fiber/pipeline/fiber_tcp_pipeline_connector_group.h"
#include "trpc/util/log/logging.h"

namespace trpc {

FiberConnectorGroupManager::FiberConnectorGroupManager(TransInfo&& trans_info) : trans_info_(std::move(trans_info)) {
  tcp_impl_.store(std::make_unique<TcpImpl>().release());
  udp_impl_.store(std::make_unique<UdpImpl>().release());

  fiber_transport_state_.store(ClientTransportState::kInitialized, std::memory_order_release);
}

FiberConnectorGroupManager::~FiberConnectorGroupManager() { Stop(); }

void FiberConnectorGroupManager::Stop() {
  std::scoped_lock _(mutex_);

  if (fiber_transport_state_.load(std::memory_order_acquire) != ClientTransportState::kInitialized) {
    return;
  }

  {
    Hazptr hazptr;
    auto tcp_impl = hazptr.Keep(&tcp_impl_);
    if (tcp_impl) {
      for (auto&& [name, group] : tcp_impl->tcp_connector_groups) {
        (void)name;
        group->Stop();
      }
    }
  }

  {
    Hazptr hazptr;
    auto udp_impl = hazptr.Keep(&udp_impl_);
    if (udp_impl) {
      if (udp_impl->udp_connector_groups[0] != nullptr) {
        udp_impl->udp_connector_groups[0]->Stop();
      }

      if (udp_impl->udp_connector_groups[1] != nullptr) {
        udp_impl->udp_connector_groups[1]->Stop();
      }
    }
  }

  fiber_transport_state_.store(ClientTransportState::kStopped, std::memory_order_release);
}

void FiberConnectorGroupManager::Destroy() {
  std::scoped_lock _(mutex_);

  if (fiber_transport_state_.load(std::memory_order_acquire) != ClientTransportState::kStopped) {
    return;
  }

  auto tcp_impl = tcp_impl_.exchange(nullptr, std::memory_order_relaxed);
  if (tcp_impl) {
    for (auto&& [name, group] : tcp_impl->tcp_connector_groups) {
      (void)name;
      group->Destroy();
      delete group;
      group = nullptr;
    }

    tcp_impl->tcp_connector_groups.clear();
    tcp_impl->Retire();
  }

  auto udp_impl = udp_impl_.exchange(nullptr, std::memory_order_relaxed);
  if (udp_impl) {
    if (udp_impl->udp_connector_groups[0] != nullptr) {
      udp_impl->udp_connector_groups[0]->Destroy();
      delete udp_impl->udp_connector_groups[0];
      udp_impl->udp_connector_groups[0] = nullptr;
    }

    if (udp_impl->udp_connector_groups[1] != nullptr) {
      udp_impl->udp_connector_groups[1]->Destroy();
      delete udp_impl->udp_connector_groups[1];
      udp_impl->udp_connector_groups[1] = nullptr;
    }
    udp_impl->Retire();
  }

  fiber_transport_state_.store(ClientTransportState::kDestroyed, std::memory_order_release);
}

FiberConnectorGroup* FiberConnectorGroupManager::Get(const NodeAddr& node_addr) {
  if (trans_info_.conn_type != ConnectionType::kUdp) {
    return GetFromTcpGroup(node_addr);
  }

  return GetFromUdpGroup(node_addr);
}

FiberConnectorGroup* FiberConnectorGroupManager::GetFromTcpGroup(const NodeAddr& node_addr) {
  const int len = 64;
  std::string endpoint(len, 0x0);
  snprintf(const_cast<char*>(endpoint.c_str()), len, "%s:%d", node_addr.ip.c_str(), node_addr.port);

  {
    Hazptr hazptr;
    auto ptr = hazptr.Keep(&tcp_impl_);
    auto it = ptr->tcp_connector_groups.find(endpoint);
    if (it != ptr->tcp_connector_groups.end()) {
      return it->second;
    }
  }

  FiberConnectorGroup* pool{nullptr};

  {
    auto new_tcp_impl = std::make_unique<TcpImpl>();

    std::scoped_lock _(mutex_);

    {
      Hazptr hazptr;
      auto ptr = hazptr.Keep(&tcp_impl_);
      auto it = ptr->tcp_connector_groups.find(endpoint);
      if (it != ptr->tcp_connector_groups.end()) {
        return it->second;
      } else {
        new_tcp_impl->tcp_connector_groups = ptr->tcp_connector_groups;
      }
    }

    pool = CreateTcpConnectorGroup(node_addr);

    new_tcp_impl->tcp_connector_groups.emplace(std::move(endpoint), pool);

    tcp_impl_.exchange(new_tcp_impl.release(), std::memory_order_acq_rel)->Retire();
  }

  return pool;
}

FiberConnectorGroup* FiberConnectorGroupManager::GetFromUdpGroup(const NodeAddr& node_addr) {
  bool is_ipv6 = (node_addr.addr_type == NodeAddr::AddrType::kIpV6);
  int index = GetUdpConnectorGroupIndex(is_ipv6);

  {
    Hazptr hazptr;
    auto udp_connector_groups = hazptr.Keep(&udp_impl_)->udp_connector_groups;
    if (udp_connector_groups[index] != nullptr) {
      return udp_connector_groups[index];
    }
  }

  FiberConnectorGroup* pool{nullptr};

  {
    auto new_udp_impl = std::make_unique<UdpImpl>();

    std::scoped_lock _(mutex_);
    {
      Hazptr hazptr;
      auto old_udp_impl = hazptr.Keep(&udp_impl_);
      if (old_udp_impl->udp_connector_groups[index]) {
        return old_udp_impl->udp_connector_groups[index];
      } else {
        new_udp_impl->udp_connector_groups[0] = old_udp_impl->udp_connector_groups[0];
        new_udp_impl->udp_connector_groups[1] = old_udp_impl->udp_connector_groups[1];
      }
    }

    pool = CreateUdpConnectorGroup(is_ipv6);

    new_udp_impl->udp_connector_groups[index] = pool;
    udp_impl_.exchange(new_udp_impl.release(), std::memory_order_acq_rel)->Retire();
  }

  return pool;
}

FiberConnectorGroup* FiberConnectorGroupManager::CreateTcpConnectorGroup(const NodeAddr& node_addr) {
  FiberConnectorGroup::Options options;
  options.trans_info = &trans_info_;
  NetworkAddress::IpType ip_type = (node_addr.addr_type == NodeAddr::AddrType::kIpV4) ? NetworkAddress::IpType::kIpV4
                                                                                      : NetworkAddress::IpType::kIpV6;
  options.peer_addr = NetworkAddress(std::string_view(node_addr.ip), node_addr.port, ip_type);

  FiberConnectorGroup* pool{nullptr};

  // tcp short connection
  if (TRPC_UNLIKELY(trans_info_.conn_type == ConnectionType::kTcpShort)) {
    pool = new FiberTcpConnPoolConnectorGroup(options);
    TRPC_ASSERT(pool->Init());
    return pool;
  }

  TRPC_LOG_DEBUG("tcp is_complex_conn:" << trans_info_.is_complex_conn << ", support_pipeline:"
                                        << trans_info_.support_pipeline << ", protocol:" << trans_info_.protocol
                                        << ", ip:" << node_addr.ip << ", port:" << node_addr.port);

  // tcp long connection
  if (trans_info_.is_complex_conn) {
    // tcp connection multiplexing, unordered multi-send and multi-receive
    pool = new FiberTcpConnComplexConnectorGroup(options);
  } else if (trans_info_.support_pipeline) {
    // tcp pipeline
    pool = new FiberTcpPipelineConnectorGroup(options);
  } else {
    // tcp pool(one-request one-connection)
    pool = new FiberTcpConnPoolConnectorGroup(options);
  }

  TRPC_ASSERT(pool->Init());

  return pool;
}

FiberConnectorGroup* FiberConnectorGroupManager::CreateUdpConnectorGroup(bool is_ipv6) {
  FiberConnectorGroup::Options options;
  options.trans_info = &trans_info_;
  options.is_ipv6 = is_ipv6;

  TRPC_LOG_DEBUG("udp is_complex_conn:" << options.is_ipv6 << ", protocol:" << trans_info_.protocol);

  FiberConnectorGroup* pool{nullptr};
  if (trans_info_.is_complex_conn) {
    pool = new FiberUdpIoComplexConnectorGroup(options);
  } else {
    pool = new FiberUdpIoPoolConnectorGroup(options);
  }

  TRPC_ASSERT(pool->Init());

  return pool;
}

}  // namespace trpc
