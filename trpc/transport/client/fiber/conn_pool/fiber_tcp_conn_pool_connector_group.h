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

#include <list>
#include <memory>
#include <mutex>

#include "trpc/transport/client/fiber/conn_pool/fiber_tcp_conn_pool_connector.h"
#include "trpc/transport/client/fiber/fiber_connector_group.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

/// @brief The implementation for fiber tcp-conn-complex connector-group
/// Manage different connection connectors under the same ip/port,
/// including the creation/release of the connection connector,
/// and the distribution of requests to connections, etc.
/// @note When used, request/response is not required to have unique id
class FiberTcpConnPoolConnectorGroup final : public FiberConnectorGroup {
 public:
  explicit FiberTcpConnPoolConnectorGroup(const FiberConnectorGroup::Options& options);

  ~FiberTcpConnPoolConnectorGroup() override;

  bool Init() override { return true; }

  void Stop() override;

  void Destroy() override;

  int SendRecv(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) override;

  Future<CTransportRspMsg> AsyncSendRecv(CTransportReqMsg* req_msg) override;

  int SendOnly(CTransportReqMsg* req_msg) override;

  void SendRecvForBackupRequest(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg,
                                OnCompletionFunction&& cb) override;

  stream::StreamReaderWriterProviderPtr CreateStream(stream::StreamOptions&& stream_options) override;

 private:
  RefPtr<FiberTcpConnPoolConnector> GetOrCreate();
  RefPtr<FiberTcpConnPoolConnector> CreateTcpConnPoolConnector(uint32_t shard_id);
  void Reclaim(int ret, RefPtr<FiberTcpConnPoolConnector>&& connector);

 private:
  FiberConnectorGroup::Options options_;

  struct alignas(hardware_destructive_interference_size) Shard {
    std::mutex lock;
    std::list<RefPtr<FiberTcpConnPoolConnector>> tcp_conns;
  };

  std::unique_ptr<Shard[]> conn_shards_;

  // The maximum number of connections that can be stored per `Shard` in `conn_shards_`
  uint32_t max_conn_per_shard_{0};

  std::atomic<uint32_t> shard_id_gen_{0};
  std::atomic<uint32_t> connector_id_gen_{0};

  std::atomic<uint32_t> long_conn_num_{0};
};

}  // namespace trpc
