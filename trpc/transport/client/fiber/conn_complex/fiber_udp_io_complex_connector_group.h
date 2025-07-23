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

#include "trpc/transport/client/fiber/conn_complex/fiber_udp_io_complex_connector.h"
#include "trpc/transport/client/fiber/fiber_connector_group.h"

namespace trpc {

/// @brief The implementation for fiber udp-io-complex connector group
/// Send requests for different ip/port services to multiplex the same batch of connectors
/// @note Using this class requires request/response to have unique id
class FiberUdpIoComplexConnectorGroup : public FiberConnectorGroup {
 public:
  explicit FiberUdpIoComplexConnectorGroup(const FiberConnectorGroup::Options& options);

  ~FiberUdpIoComplexConnectorGroup() override;

  bool Init() override { return true; }

  void Stop() override;

  void Destroy() override;

  int SendRecv(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) override;

  Future<CTransportRspMsg> AsyncSendRecv(CTransportReqMsg* req_msg) override;

  int SendOnly(CTransportReqMsg* req_msg) override;

 private:
  RefPtr<FiberUdpIoComplexConnector> CreateUdpIoComplexConnector();
  RefPtr<FiberUdpIoComplexConnector> Get();

 private:
  FiberConnectorGroup::Options options_;

  size_t max_conn_num_;

  std::vector<RefPtr<FiberUdpIoComplexConnector>> udp_connectors_;
};

}  // namespace trpc
