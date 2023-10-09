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

#include <functional>
#include <string>
#include <utility>

#include "trpc/common/future/future.h"
#include "trpc/runtime/iomodel/reactor/common/network_address.h"
#include "trpc/stream/stream.h"
#include "trpc/transport/client/client_transport_message.h"
#include "trpc/transport/client/fiber/common/call_context.h"
#include "trpc/transport/client/trans_info.h"

namespace trpc {

/// @brief Manager the connectors that access to the backend ip/port node
/// Different connection modes have different implementations,
/// and its public interface has the same meaning as the fiber transport interface.
class FiberConnectorGroup {
 public:
  struct Options {
    TransInfo* trans_info;
    NetworkAddress peer_addr;
    bool is_ipv6;
  };

  virtual ~FiberConnectorGroup() = default;

  virtual bool Init() = 0;

  virtual void Stop() = 0;
  
  virtual void Destroy() = 0;

  virtual int SendRecv(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) = 0;

  virtual Future<CTransportRspMsg> AsyncSendRecv(CTransportReqMsg* req_msg) = 0;

  virtual int SendOnly(CTransportReqMsg* req_msg) = 0;

  virtual void SendRecvForBackupRequest(
      CTransportReqMsg* req_msg,
      CTransportRspMsg* rsp_msg,
      OnCompletionFunction&& cb) {
    int ret = TrpcRetCode::TRPC_CLIENT_CONNECT_ERR;
    std::string err_msg("backup-request is not implement");

    cb(ret, std::move(err_msg));
  }

  virtual stream::StreamReaderWriterProviderPtr CreateStream(stream::StreamOptions&&) {
    TRPC_FMT_ERROR("stream is not implement.");
    return nullptr;
  }
};

}  // namespace trpc
