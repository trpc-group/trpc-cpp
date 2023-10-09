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

#include "trpc/stream/util.h"

namespace trpc::stream {

namespace internal {

ServerContextPtr BuildServerContext(const ConnectionPtr& conn, const ServerCodecPtr& codec,
                                    uint64_t recv_timestamp_us) {
  ServerContextPtr context = MakeRefCounted<ServerContext>();
  // Recv timestamp.
  context->SetRecvTimestampUs(recv_timestamp_us);

  // Connection.
  context->SetConnectionId(conn->GetConnId());
  context->SetFd(conn->GetFd());
  ServerContext::NetType network_type = ServerContext::NetType::kTcp;
  if (conn->GetConnType() == ConnectionType::kUdp) {
    network_type = ServerContext::NetType::kUdp;
  } else if (conn->GetConnType() == ConnectionType::kOther) {
    network_type = ServerContext::NetType::kOther;
  }
  context->SetNetType(network_type);
  context->SetPort(conn->GetPeerPort());
  context->SetIp(conn->GetPeerIp());

  // Codec.
  context->SetServerCodec(codec.get());
  context->SetRequestMsg(codec->CreateRequestObject());
  context->SetResponseMsg(codec->CreateResponseObject());

  return context;
}

}  // namespace internal

}  // namespace trpc::stream
