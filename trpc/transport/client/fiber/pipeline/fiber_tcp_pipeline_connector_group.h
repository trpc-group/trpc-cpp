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

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include "trpc/transport/client/fiber/pipeline/fiber_tcp_pipeline_connector.h"
#include "trpc/transport/client/fiber/fiber_connector_group.h"
#include "trpc/util/hazptr/hazptr_object.h"
#include "trpc/util/hazptr/hazptr.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

/// @brief The implementation for fiber tcp-conn-pipeline connector-group
/// Manage different connection connectors under the same ip/port,
/// including the creation/release of the connection connector,
/// and the distribution of requests to connections, etc.
/// @note When used, the order in which the responses are returned is required to
/// be the same as the order in which the requests were sent
class FiberTcpPipelineConnectorGroup final : public FiberConnectorGroup {
 public:
  explicit FiberTcpPipelineConnectorGroup(const FiberConnectorGroup::Options& options);

  ~FiberTcpPipelineConnectorGroup() override;

  bool Init() override { return true; }

  void Stop() override;

  void Destroy() override;

  int SendRecv(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) override;

  Future<CTransportRspMsg> AsyncSendRecv(CTransportReqMsg* req_msg) override;

  int SendOnly(CTransportReqMsg* req_msg) override;

  void SendRecvForBackupRequest(
      CTransportReqMsg* req_msg,
      CTransportRspMsg* rsp_msg,
      OnCompletionFunction&& cb) override;

 private:
  RefPtr<FiberTcpPipelineConnector> GetOrCreate();
  RefPtr<FiberTcpPipelineConnector> CreateTcpPipelineConnector(uint64_t conn_id);
  void Reclaim(int ret, RefPtr<FiberTcpPipelineConnector>& connector);

 private:
  struct Impl : HazptrObject<Impl> {
    std::vector<RefPtr<FiberTcpPipelineConnector>> tcp_conns;
  };

  FiberConnectorGroup::Options options_;

  size_t max_conn_num_;

  std::atomic<Impl*> impl_;

  std::mutex mutex_;
};

}  // namespace trpc
