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

#include "trpc/transport/client/future/future_tcp_connector_group_manager.h"

#include <string_view>

#include "trpc/transport/client/future/common/future_connector_options.h"
#include "trpc/transport/client/future/conn_complex/future_tcp_conn_complex_connector_group.h"
#include "trpc/transport/client/future/conn_pool/future_tcp_conn_pool_connector_group.h"
#include "trpc/transport/client/future/pipeline/future_tcp_pipeline_connector_group.h"
#include "trpc/transport/common/transport_message_common.h"

namespace trpc {

FutureTcpConnectorGroupManager::FutureTcpConnectorGroupManager(const Options& options)
    : FutureConnectorGroupManager(options) {}

void FutureTcpConnectorGroupManager::Stop() {
  if (options_.trans_info->is_complex_conn && timer_id_ != kInvalidTimerId) {
    options_.reactor->CancelTimer(timer_id_);
    timer_id_ = kInvalidTimerId;
  }

  for (auto& item : connector_groups_) {
    item.second->Stop();
  }
}

void FutureTcpConnectorGroupManager::Destroy() {
  for (auto& item : connector_groups_) {
    item.second->Destroy();
  }
}

bool FutureTcpConnectorGroupManager::CreateTimer() {
  if (!is_create_timer_) {
    if (options_.trans_info->is_complex_conn) {
      shared_msg_timeout_handler_ =
          std::make_unique<FutureConnComplexMessageTimeoutHandler>(options_.trans_info->rsp_dispatch_function);
      TRPC_ASSERT(shared_msg_timeout_handler_ != nullptr);
      auto timeout_check_interval = options_.trans_info->request_timeout_check_interval;
      timer_id_ = options_.reactor->AddTimerAfter(
          0, timeout_check_interval, [this]() { shared_msg_timeout_handler_->DoTimeout(); });
      if (timer_id_ == kInvalidTimerId) {
        TRPC_FMT_ERROR("create timer failed.");
        return false;
      }

      is_create_timer_ = true;
    }
  }

  return true;
}

bool FutureTcpConnectorGroupManager::GetOrCreateConnector(const NodeAddr& node_addr,
                                                          Promise<uint64_t>& promise) {
  if (!CreateTimer()) {
    return false;
  }

  FutureConnectorGroup* connector_group = GetConnectorGroup(node_addr);
  if (connector_group) {
    return connector_group->GetOrCreateConnector(node_addr, promise);
  }

  return false;
}

FutureConnectorGroup* FutureTcpConnectorGroupManager::GetConnectorGroup(const NodeAddr& node_addr) {
  if (!CreateTimer()) {
    return nullptr;
  }

  size_t len = 64;
  std::string endpoint_id(len, 0x0);
  snprintf(const_cast<char*>(endpoint_id.c_str()), len, "%s:%d", node_addr.ip.c_str(), node_addr.port);

  auto it = connector_groups_.find(endpoint_id);
  if (it != connector_groups_.end()) {
    return it->second.get();
  } else {
    FutureConnectorGroupOptions connector_group_options;
    connector_group_options.reactor = options_.reactor;
    connector_group_options.trans_info = options_.trans_info;
    connector_group_options.peer_addr =
        NetworkAddress(std::string_view(node_addr.ip), node_addr.port,
                       node_addr.addr_type == NodeAddr::AddrType::kIpV6 ? NetworkAddress::IpType::kIpV6
                                                                        : NetworkAddress::IpType::kIpV4);
    connector_group_options.ip_type =
        (node_addr.addr_type == NodeAddr::AddrType::kIpV6 ? NetworkAddress::IpType::kIpV6
                                                          : NetworkAddress::IpType::kIpV4);
    connector_group_options.conn_group_manager = this;

    std::unique_ptr<FutureConnectorGroup> connector_group;
    if (options_.trans_info->is_complex_conn) {
      connector_group = std::make_unique<FutureTcpConnComplexConnectorGroup>(std::move(connector_group_options),
                                                                             *shared_msg_timeout_handler_);
    } else if (options_.trans_info->support_pipeline) {
      connector_group = std::make_unique<FutureTcpPipelineConnectorGroup>(std::move(connector_group_options));
    } else {
      connector_group = std::make_unique<FutureTcpConnPoolConnectorGroup>(std::move(connector_group_options));
    }

    bool ret = connector_group->Init();
    if (ret) {
      FutureConnectorGroup* ptr = connector_group.get();
      connector_groups_[endpoint_id] = std::move(connector_group);

      return ptr;
    }

    return nullptr;
  }
}

void FutureTcpConnectorGroupManager::DelConnectorGroup(const NodeAddr& node_addr) {
  size_t len = 64;
  std::string endpoint_id(len, 0x0);
  snprintf(const_cast<char*>(endpoint_id.c_str()), len, "%s:%d", node_addr.ip.c_str(), node_addr.port);
  auto it = connector_groups_.find(endpoint_id);
  if (it != connector_groups_.end()) {
    it->second->Stop();
    it->second->Destroy();
    connector_groups_.erase(it);
  }
}

void FutureTcpConnectorGroupManager::Disconnect(const std::string& target_ip) {
  for (auto it = connector_groups_.begin(); it != connector_groups_.end();) {
    if (it->first.find(target_ip) != std::string::npos) {
      it->second->Stop();
      it->second->Destroy();
      it = connector_groups_.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace trpc
