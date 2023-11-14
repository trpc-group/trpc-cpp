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

#include "trpc/transport/client/future/common/future_transport_adapter.h"

#include "trpc/runtime/runtime.h"
#include "trpc/transport/client/future/future_tcp_connector_group_manager.h"
#include "trpc/transport/client/future/future_udp_connector_group_manager.h"
#include "trpc/util/thread/latch.h"

namespace trpc {

FutureTransportAdapter::FutureTransportAdapter(const Options& options) : options_(options) {
  FutureConnectorGroupManager::Options manager_options;
  manager_options.reactor = options_.reactor;
  manager_options.trans_info = options_.trans_info;
  if (options_.trans_info->conn_type != ConnectionType::kUdp) {
    group_manager_ = std::make_unique<FutureTcpConnectorGroupManager>(manager_options);
  } else {
    group_manager_ = std::make_unique<FutureUdpConnectorGroupManager>(manager_options);
  }

  transport_state_.store(ClientTransportState::kInitialized, std::memory_order_release);
}

FutureTransportAdapter::~FutureTransportAdapter() { Stop(); }

FutureConnectorGroup* FutureTransportAdapter::GetConnectorGroup(const NodeAddr& addr) {
  return group_manager_->GetConnectorGroup(addr);
}

void FutureTransportAdapter::Stop() {
  if (group_manager_ == nullptr) {
    return;
  }

  if (transport_state_.load(std::memory_order_acquire) != ClientTransportState::kInitialized) {
    return;
  }

  auto l = std::make_shared<Latch>(1);

  Reactor::Task task = [this, l]() {
    group_manager_->Stop();
    transport_state_.store(ClientTransportState::kStopped, std::memory_order_release);
    l->count_down();
  };

  TRPC_ASSERT(options_.reactor != nullptr);
  bool ret = options_.reactor->SubmitTask2(std::move(task));
  if (ret) {
    if (options_.reactor->GetCurrentTlsReactor() != options_.reactor) {
      /// @note To avoid thread death hang in pure client scenario, where ServiceProxy created by user.
      l->wait_for(std::chrono::seconds(5));
    }
  } else {
    TRPC_FMT_ERROR("Stop failed.");
  }
}

void FutureTransportAdapter::Destroy() {
  if (group_manager_ == nullptr) {
    return;
  }

  if (transport_state_.load(std::memory_order_acquire) != ClientTransportState::kStopped) {
    return;
  }

  group_manager_->Destroy();
  group_manager_.reset();

  transport_state_.store(ClientTransportState::kDestroyed, std::memory_order_release);
}

bool FutureTransportAdapter::GetOrCreateConnector(const NodeAddr& node_addr, Promise<uint64_t>& promise) {
  return group_manager_->GetOrCreateConnector(node_addr, promise);
}

void FutureTransportAdapter::Disconnect(const std::string& target_ip) {
  if (group_manager_ == nullptr) {
    return;
  }

  Reactor::Task task = [this, target_ip]() {
    group_manager_->Disconnect(target_ip);
  };

  bool ret = options_.reactor->SubmitTask2(std::move(task));
  if (!ret) {
    TRPC_FMT_ERROR("Disconnect {} failed.", target_ip);
  }
}

}  // namespace trpc
