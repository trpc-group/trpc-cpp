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

#include "trpc/runtime/iomodel/reactor/fiber/fiber_udp_transceiver.h"
#include "trpc/transport/client/client_transport_message.h"
#include "trpc/transport/client/fiber/common/sharded_call_map.h"
#include "trpc/transport/client/trans_info.h"
#include "trpc/util/align.h"
#include "trpc/util/object_pool/object_pool_ptr.h"

namespace trpc {

/// @brief The implementation for fiber udp-io-complex connector
class FiberUdpIoComplexConnector final : public RefCounted<FiberUdpIoComplexConnector> {
 public:
  struct Options {
    uint64_t conn_id;
    TransInfo* trans_info;
    bool is_ipv6;
  };

  explicit FiberUdpIoComplexConnector(const Options& options);

  ~FiberUdpIoComplexConnector();

  bool Init();

  void Stop();

  void Destroy();

  void SendReqMsg(CTransportReqMsg* req_msg);

  uint64_t GetConnId() { return options_.conn_id; }

  void SaveCallContext(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg, OnCompletionFunction&& cb);

 private:
  bool MessageHandleFunction(const ConnectionPtr& conn, std::deque<std::any>& rsp_list);
  bool CreateFiberUdpTransceiver(uint64_t conn_id);
  uint64_t CreateTimer(CTransportReqMsg* req_msg);
  void OnTimeout(uint32_t req_id, std::string&& ip, int port);
  void DispatchResponse(object_pool::LwUniquePtr<CallContext> ctx);
  void DispatchException(uint32_t request_id, int ret, std::string&& err_msg, std::string&& ip, int port);
  void DispatchException(object_pool::LwUniquePtr<CallContext> ctx, int ret, std::string&& err_msg);
  void Send(CTransportReqMsg* req_msg);
  void ClearResource();

 private:
  Options options_;

  RefPtr<FiberUdpTransceiver> connection_;

  RefPtr<CallMap> call_map_;
};

}  // namespace trpc
