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
#include <string>

#include "trpc/stream/stream.h"
#include "trpc/transport/client/client_transport.h"
#include "trpc/transport/client/fiber/fiber_connector_group_manager.h"
#include "trpc/transport/common/transport_message_common.h"

namespace trpc {

/// @brief The implementation of client fiber transport
/// Currently, the mode of connection supports the following:
/// 1.Support tcp connection multiplexing, unordered multi-send and multi-receive
///   (request and response must have a unique id)
/// 2.tcp connection pool, one-request one-connection
///   (request and response do not have a unique id)
/// 3.tcp pipeline
///   (requests are sent in order, and responses are required to be returned in the corresponding order of requests)
/// 4.udp io multiplexing
///   (request and response must have a unique id)
/// 5.udp pool
///   (request and response do not have a unique id)
/// @note Before FiberTransport is destructed, you need to call stop first, and then use Destroy.
/// SendRecv/AsyncSendRecv/SendOnly interface thread/fiber security
class FiberTransport final : public ClientTransport {
 public:
  std::string Name() const override { return "fiber_transport"; }

  std::string Version() const override { return "0.1.0"; }

  int Init(Options&& params) override;

  /// @brief Stop and close the inner connectins
  void Stop() override;

  /// @brief Destroy the inner resources
  void Destroy() override;

  /// @brief Fiber sync send request message and recv response message
  /// @note  If call in the fiber worker thread will not block the current thread,
  /// If call outside the fiber worker thread will block the current thread.
  /// `req_msg` and `rsp_msg` must be created by trpc::object_pool::New<CTransportReqMsg>
  int SendRecv(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg) override;

  /// @brief Fiber async send request message and recv response message
  /// @note  Call in the fiber worker thread or outside the fiber worker thread will not block the current thread.
  /// `req_msg` must be created by trpc::object_pool::New<CTransportReqMsg>
  Future<CTransportRspMsg> AsyncSendRecv(CTransportReqMsg* req_msg) override;

  /// @brief Fiber one-way invoke
  /// `req_msg` must be created by trpc::object_pool::New<CTransportReqMsg>
  int SendOnly(CTransportReqMsg* req_msg) override;

  /// @brief Create stream
  stream::StreamReaderWriterProviderPtr CreateStream(const NodeAddr& addr,
                                                     stream::StreamOptions&& stream_options) override;

 private:
  int SendRecvFromOutSide(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg);
  Future<CTransportRspMsg> AsyncSendRecvFromOutSide(CTransportReqMsg* req_msg);
  int SendOnlyFromOutSide(CTransportReqMsg* req_msg);
  int SendRecvForBackupRequest(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg);

 private:
  std::unique_ptr<FiberConnectorGroupManager> connector_group_manager_;
};

}  // namespace trpc
