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
#include <mutex>
#include <vector>

#include "trpc/transport/client/fiber/conn_pool/fiber_udp_io_pool_connector.h"
#include "trpc/transport/client/fiber/fiber_connector_group.h"
#include "trpc/util/lockfree_queue.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

/// @brief The implementation for fiber udp-io-complex connector-group
/// Manage different connection connectors under the same ip/port,
/// including the creation/release of the connection connector,
/// and the distribution of requests to connections, etc.
/// @note When used, request/response is not required to have unique id
class FiberUdpIoPoolConnectorGroup final : public FiberConnectorGroup {
 public:
  explicit FiberUdpIoPoolConnectorGroup(const FiberConnectorGroup::Options& options);

  ~FiberUdpIoPoolConnectorGroup() override;

  bool Init() override { return true; }

  void Stop() override;

  void Destroy() override;

  int SendRecv(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) override;

  Future<CTransportRspMsg> AsyncSendRecv(CTransportReqMsg* req_msg) override;

  int SendOnly(CTransportReqMsg* req_msg) override;

 private:
  uint64_t GetAvailableId();
  RefPtr<FiberUdpIoPoolConnector> GetOrCreate();
  RefPtr<FiberUdpIoPoolConnector> CreateUdpIoPoolConnector(uint32_t id);
  void Reclaim(uint64_t id);
  void Reclaim(int ret, RefPtr<FiberUdpIoPoolConnector>& connector);

 private:
  FiberConnectorGroup::Options options_;

  std::vector<RefPtr<FiberUdpIoPoolConnector>> udp_pool_;

  LockFreeQueue<uint64_t> available_ids_;
};

}  // namespace trpc
