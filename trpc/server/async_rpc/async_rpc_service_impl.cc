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

#include "trpc/server/async_rpc/async_rpc_service_impl.h"

#include "trpc/util/log/logging.h"

namespace trpc {

void AsyncRpcServiceImpl::Dispatch(const ServerContextPtr& context, const ProtocolPtr& req,
    ProtocolPtr& rsp) noexcept {
  RpcMethodHandlerInterface* method_handler = GetUnaryRpcMethodHandler(context->GetFuncName());
  if (!method_handler) {
    HandleNoFuncError(context);
    return;
  }

  method_handler->Execute(context);
}

void AsyncRpcServiceImpl::DispatchStream(const ServerContextPtr& context) noexcept {
  context->SetService(this);
  RpcMethodHandlerInterface* handler = GetStreamRpcMethodHandler(context->GetFuncName());
  if (!handler) {
    HandleNoFuncError(context);
    // StreamReaderWriterProvider object will be moved from context.
    // NOTE: stream options will store context object, and the server context will also set stream provider.
    context->GetStreamReaderWriterProvider();
    return;
  }

  handler->Execute(context);
}

}  // namespace trpc
