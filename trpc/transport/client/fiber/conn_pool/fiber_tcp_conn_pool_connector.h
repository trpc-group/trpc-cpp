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
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "trpc/runtime/iomodel/reactor/fiber/fiber_tcp_connection.h"
#include "trpc/transport/client/client_transport_message.h"
#include "trpc/transport/client/fiber/common/call_context.h"
#include "trpc/transport/client/trans_info.h"
#include "trpc/util/align.h"
#include "trpc/util/object_pool/object_pool_ptr.h"

namespace trpc {

/// @brief The implementation for fiber tcp-conn-pool connector
/// @note  Once the request on the instance of this class times out,
/// the connector instance object can no longer be used
class FiberTcpConnPoolConnector final : public RefCounted<FiberTcpConnPoolConnector> {
 public:
  struct Options {
    uint64_t conn_id;
    TransInfo* trans_info;
    NetworkAddress* peer_addr;
  };

  explicit FiberTcpConnPoolConnector(const Options& options);

  ~FiberTcpConnPoolConnector();

  bool Init();

  void Stop();

  void Destroy();

  bool SaveCallContext(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg, OnCompletionFunction&& cb);

  void SendReqMsg(CTransportReqMsg* req_msg);

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
  uint32_t GetRequestMergeCount();
  object_pool::LwUniquePtr<CallContext> TryGetCallContext();
  object_pool::LwUniquePtr<CallContext> TryGetCallContext(uint32_t req_id);
  uint64_t CreateTimer(uint32_t request_id, uint32_t timeout);
  void OnTimeout(uint32_t request_id);
  void Send(CTransportReqMsg* req_msg);
  void ClearResource();
  void ClearReqMsg();

 private:
  Options options_;

  std::atomic<bool> healthy_{false};

  std::atomic<bool> cleanup_{false};

  RefPtr<FiberTcpConnection> connection_;

  std::mutex mutex_;

  object_pool::LwUniquePtr<CallContext> ctx_;
};

}  // namespace trpc
