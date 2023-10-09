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

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include "trpc/transport/client/fiber/conn_complex/fiber_tcp_conn_complex_connector.h"
#include "trpc/transport/client/fiber/fiber_connector_group.h"
#include "trpc/util/hazptr/hazptr.h"
#include "trpc/util/hazptr/hazptr_object.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

/// @brief The implementation for fiber tcp-conn-complex connector-group
/// Manage different connection connectors under the same ip/port,
/// including the creation/release of the connection connector,
/// and the distribution of requests to connections, etc.
/// @note Using this class requires request/response to have unique id
class FiberTcpConnComplexConnectorGroup final : public FiberConnectorGroup {
 public:
  explicit FiberTcpConnComplexConnectorGroup(const FiberConnectorGroup::Options& options);

  ~FiberTcpConnComplexConnectorGroup() override;

  bool Init() override { return true; }

  void Stop() override;

  void Destroy() override;

  int SendRecv(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) override;

  Future<CTransportRspMsg> AsyncSendRecv(CTransportReqMsg* req_msg) override;

  int SendOnly(CTransportReqMsg* req_msg) override;

  void SendRecvForBackupRequest(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg,
                                OnCompletionFunction&& cb) override;

  stream::StreamReaderWriterProviderPtr CreateStream(stream::StreamOptions&& stream_options) override;

  bool DelConnector(FiberTcpConnComplexConnector* connector);

 private:
  RefPtr<FiberTcpConnComplexConnector> GetOrCreate();
  RefPtr<FiberTcpConnComplexConnector> CreateTcpConnComplexConnector(uint64_t conn_id);

 private:
  struct Impl : HazptrObject<Impl> {
    std::vector<RefPtr<FiberTcpConnComplexConnector>> tcp_conns;
  };

  FiberConnectorGroup::Options options_;

  size_t max_conn_num_;

  std::atomic<Impl*> impl_;

  std::mutex mutex_;
};

}  // namespace trpc
