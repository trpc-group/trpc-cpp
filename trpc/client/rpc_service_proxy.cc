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

#include "trpc/client/rpc_service_proxy.h"

namespace trpc {

Status RpcServiceProxy::PbSerializedReqUnaryInvoke(const ClientContextPtr& context,
                                                   NoncontiguousBuffer&& req,
                                                   google::protobuf::Message* rsp) {
  TRPC_ASSERT(context->GetRequest() != nullptr);

  context->SetReqEncodeType(serialization::kPbType);
  context->SetReqEncodeDataType(serialization::kNonContiguousBufferNoop);

  FillClientContext(context);

  context->SetRequestData(const_cast<NoncontiguousBuffer*>(&req));
  context->SetResponseData(rsp);

  int filter_ret = RunFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_ret == 0) {
    UnaryInvokeImp<NoncontiguousBuffer, google::protobuf::Message>(context, req, rsp);
  }
  RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);

  context->SetRequestData(nullptr);
  context->SetResponseData(nullptr);

  return context->GetStatus();
}

}  // namespace trpc
