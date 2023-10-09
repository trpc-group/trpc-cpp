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
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "trpc/runtime/iomodel/reactor/fiber/fiber_tcp_connection.h"
#include "trpc/transport/client/client_transport_message.h"
#include "trpc/transport/client/fiber/common/sharded_call_map.h"
#include "trpc/transport/client/trans_info.h"
#include "trpc/util/align.h"
#include "trpc/util/lockfree_queue.h"
#include "trpc/util/object_pool/object_pool_ptr.h"

namespace trpc {

class FiberTcpPipelineConnectorGroup;

/// @brief The implementation for fiber tcp-conn-pipeline connector
/// @note  Once the request on the instance of this class times out,
/// the connector instance object can no longer be used
class FiberTcpPipelineConnector final : public RefCounted<FiberTcpPipelineConnector> {
 public:
  struct Options {
    uint64_t conn_id;
    TransInfo* trans_info;
    NetworkAddress* peer_addr;
    FiberTcpPipelineConnectorGroup* connector_group;
  };

  explicit FiberTcpPipelineConnector(const Options& options);

  ~FiberTcpPipelineConnector();

  bool Init();

  void Stop();

  void Destroy();

  bool SaveCallContext(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg, OnCompletionFunction&& cb);

  void SendReqMsg(CTransportReqMsg* req_msg);

  void SendOnly(CTransportReqMsg* req_msg);

  void SetHealthy(bool value) { healthy_.store(value, std::memory_order_release); }

  bool IsHealthy() const { return healthy_.load(std::memory_order_acquire); }

  bool IsConnIdleTimeout() const;

  void CloseConnection();

  uint64_t GetConnId() { return options_.conn_id; }

  Connection* GetConnection() { return connection_.Get(); }

 private:
  bool MessageHandleFunction(const ConnectionPtr& conn, std::deque<std::any>& rsp_list);
  void ConnectionCleanFunction(Connection* conn);
  bool CreateFiberTcpConnection(uint64_t conn_id);
  void DispatchResponse(object_pool::LwUniquePtr<CallContext> ctx);
  void DispatchException(uint32_t request_id, int ret, std::string&& err_msg);
  void DispatchException(object_pool::LwUniquePtr<CallContext> ctx, int ret, std::string&& err_msg);
  uint64_t CreateTimer(uint32_t request_id, uint32_t timeout);
  void OnTimeout(uint32_t req_id);
  void Send(CTransportReqMsg* req_msg, bool is_oneway);
  void NotifyPipelineSendMsg(uint32_t request_id);
  void ClearResource();
  void ClearReqMsg();
  std::optional<uint32_t> GetMatchedRequestId();

 private:
  Options options_;

  std::atomic<bool> healthy_{false};

  std::atomic<bool> cleanup_{false};

  RefPtr<FiberTcpConnection> connection_;

  RefPtr<CallMap> call_map_;

  // FIFO queue for request
  LockFreeQueue<uint32_t> send_request_id_queue_;
};

}  // namespace trpc
