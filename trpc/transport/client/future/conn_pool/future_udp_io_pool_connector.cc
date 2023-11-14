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

#include "trpc/transport/client/future/conn_pool/future_udp_io_pool_connector.h"

#include <string>
#include <utility>

#include "trpc/filter/filter.h"
#include "trpc/transport/client/future/conn_pool/future_conn_pool_connection_handler_factory.h"
#include "trpc/util/time.h"

namespace trpc {

bool FutureUdpIoPoolConnector::Init() {
  auto conn_event_handler = FutureConnPoolConnectionHandlerFactory::GetIntance()->Create(
    options_.group_options->trans_info->protocol,
    options_,
    msg_timeout_handler_
  );

  FutureUdpIoPoolConnectionHandler* handler =
    dynamic_cast<FutureUdpIoPoolConnectionHandler*>(conn_event_handler.get());
  TRPC_ASSERT(handler && "dynamic_cast to FutureUdpIoPoolConnectionHandler error");
  handler->SetConnPool(conn_pool_);

  connection_ = future::CreateUdpTransceiver(options_, std::move(conn_event_handler));
  if (connection_ == nullptr) {
    return false;
  }

  return true;
}

void FutureUdpIoPoolConnector::Stop() {
  if (!connection_) return;

  connection_->DoClose(false);
}

void FutureUdpIoPoolConnector::Destroy() {
  if (!connection_) return;

  connection_.Reset();
}

int FutureUdpIoPoolConnector::SendReqMsg(CTransportReqMsg* req_msg) {
  if (!msg_timeout_handler_.IsSendQueueEmpty()) {
    std::string error = "send queue not empty, peer addr:";
    error += options_.group_options->peer_addr.ToString();
    future::DispatchException(req_msg, TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR, std::move(error),
                              options_.group_options->trans_info->rsp_dispatch_function);
    return -1;
  }

  msg_timeout_handler_.PushToSendQueue(req_msg);
  int ret = future::SendUdpMsg(req_msg, connection_.Get());
  if (!ret) return 0;

  req_msg = msg_timeout_handler_.PopFromSendQueue();
  if (req_msg) {
    std::string error = "network send error, peer addr:";
    error += options_.group_options->peer_addr.ToString();
    future::DispatchException(req_msg, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, std::move(error),
                              options_.group_options->trans_info->rsp_dispatch_function);
  }

  return -1;
}

int FutureUdpIoPoolConnector::SendOnly(CTransportReqMsg* req_msg) {
  if (!connection_) return -1;

  int ret = future::SendUdpMsg(req_msg, connection_.Get());
  conn_pool_->RecycleConnectorId(connection_->GetConnId());
  if (ret == 0) return 0;

  return -1;
}

void FutureUdpIoPoolConnector::HandleSendQueueTimeout(CTransportReqMsg* req_msg) {
  if (!msg_timeout_handler_.HandleSendQueueTimeout(req_msg)) return;

  msg_timeout_handler_.NeedCheckQueueWhenExit(false);

  // If request timeout, must close connection to avoid packet collision.
  std::unique_ptr<FutureConnector> connector = conn_pool_->GetAndDelConnector(options_.conn_id);
  options_.group_options->reactor->SubmitInnerTask([connector = std::move(connector)]() {
    if (connector) {
      connector->Stop();
      connector->Destroy();
    }
  });
}

}  // namespace trpc
