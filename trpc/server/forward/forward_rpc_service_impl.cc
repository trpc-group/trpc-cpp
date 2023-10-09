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

#include "trpc/server/forward/forward_rpc_service_impl.h"

#include "trpc/server/server_context.h"
#include "trpc/util/log/logging.h"

namespace trpc {

void ForwardRpcServiceImpl::Dispatch(const ServerContextPtr& context,
    const ProtocolPtr& req, ProtocolPtr& rsp) noexcept {
  RpcMethodHandlerInterface* method_handler = GetUnaryRpcMethodHandler(kTransparentRpcName);
  if (!method_handler) {
    std::string err_msg = kTransparentRpcName;
    err_msg += " method handler not register";

    TRPC_ASSERT(context->GetServerCodec() != nullptr);

    context->GetStatus().SetFrameworkRetCode(
        context->GetServerCodec()->GetProtocolRetCode(trpc::codec::ServerRetCode::NOT_FUN_ERROR));
    context->GetStatus().SetErrorMessage(std::move(err_msg));
    return;
  }

  NoncontiguousBuffer response_body;
  method_handler->Execute(context, req->GetNonContiguousProtocolBody(), response_body);

  if (context->IsResponse()) {
    rsp->SetNonContiguousProtocolBody(std::move(response_body));
  }
}

}  // namespace trpc
