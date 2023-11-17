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

#include "trpc/transport/client/future/conn_complex/future_udp_io_complex_connector_group.h"

namespace trpc {

FutureUdpIoComplexConnectorGroup::FutureUdpIoComplexConnectorGroup(FutureConnectorGroupOptions&& options)
  : FutureConnectorGroup(std::move(options)) {
  FutureConnectorOptions connector_options;
  connector_options.conn_id = 0;
  connector_options.group_options = &options_;
  connector_ = std::make_unique<FutureUdpIoComplexConnector>(std::move(connector_options));
}

bool FutureUdpIoComplexConnectorGroup::Init() {
  return connector_->Init();
}

void FutureUdpIoComplexConnectorGroup::Stop() {
  connector_->Stop();
}

void FutureUdpIoComplexConnectorGroup::Destroy() {
  connector_->Destroy();
}

int FutureUdpIoComplexConnectorGroup::SendReqMsg(CTransportReqMsg* req_msg) {
  return connector_->SendReqMsg(req_msg);
}

int FutureUdpIoComplexConnectorGroup::SendOnly(CTransportReqMsg* req_msg) {
  return connector_->SendOnly(req_msg);
}

}  // namespace trpc
