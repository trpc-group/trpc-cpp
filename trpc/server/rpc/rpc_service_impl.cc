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

#include "trpc/server/rpc/rpc_service_impl.h"

#include <string>
#include <utility>

#include "trpc/codec/codec_helper.h"
#include "trpc/codec/protocol.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/stream/stream_handler.h"
#include "trpc/util/log/logging.h"

namespace trpc {

void RpcServiceImpl::Dispatch(const ServerContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp) noexcept {
  RpcMethodHandlerInterface* method_handler = GetUnaryRpcMethodHandler(context->GetFuncName());
  if (!method_handler) {
    HandleNoFuncError(context);
    return;
  }

  NoncontiguousBuffer response_body;
  method_handler->Execute(context, req->GetNonContiguousProtocolBody(), response_body);

  if (context->IsResponse()) {
    rsp->SetNonContiguousProtocolBody(std::move(response_body));
  }
}

void RpcServiceImpl::DispatchStream(const ServerContextPtr& context) noexcept {
  context->SetService(this);
  RpcMethodHandlerInterface* handler = GetStreamRpcMethodHandler(context->GetFuncName());
  if (!handler) {
    HandleNoFuncError(context);
    // StreamReaderWriterProvider object will be moved from context.
    // NOTE: stream options will store context object, and the server context will also set stream provider.
    context->GetStreamReaderWriterProvider();
    return;
  }

  // Sets window size of stream.
  stream::StreamReaderWriterProviderPtr stream_provider = context->GetStreamReaderWriterProvider();
  stream_provider->GetMutableStreamOptions()->stream_max_window_size = GetServiceAdapterOption().stream_max_window_size;
  context->SetStreamReaderWriterProvider(std::move(stream_provider));
  bool start_fiber = StartFiberDetached([handler, context] { handler->Execute(context); });

  if (TRPC_UNLIKELY(!start_fiber)) {
    TRPC_FMT_INFO_IF(TRPC_EVERY_N(1000),
                     "StartFiberDetached to execute DispatchStream faild, discard this "
                     "stream, please retry, discard this request,please retry latter");

    context->GetStatus().SetFrameworkRetCode(
        context->GetServerCodec()->GetProtocolRetCode(trpc::codec::ServerRetCode::OVERLOAD_ERROR));
    context->GetStatus().SetErrorMessage("server overload,too many fiber to dispatch stream.");
    // StreamReaderWriterProvider object will be moved from context.
    // NOTE: stream options will store context object, and the server context will also set stream provider.
    context->GetStreamReaderWriterProvider();
    return;
  }
}

}  // namespace trpc
