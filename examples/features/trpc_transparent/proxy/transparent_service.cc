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

#include "examples/features/trpc_transparent/proxy/transparent_service.h"

#include <memory>
#include <string>

#include "trpc/client/make_client_context.h"
#include "trpc/client/trpc_client.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/log/trpc_log.h"
#include "trpc/server/forward/forward_rpc_method_handler.h"

namespace examples::transparent {

TransparentServiceImpl::TransparentServiceImpl() {
  // register transparent service method handler
  // method name must be ::trpc::kTransparentRpcName
  AddRpcServiceMethod(new ::trpc::RpcServiceMethod(
      ::trpc::kTransparentRpcName, ::trpc::MethodType::UNARY,
      new ::trpc::ForwardRpcMethodHandler(std::bind(&TransparentServiceImpl::Forward, this, std::placeholders::_1,
                                                    std::placeholders::_2, std::placeholders::_3))));

  proxy_ptr_ =
      ::trpc::GetTrpcClient()->GetProxy<::trpc::RpcServiceProxy>("trpc.test.helloworld.Greeter");
}

::trpc::Status TransparentServiceImpl::Forward(::trpc::ServerContextPtr context, ::trpc::NoncontiguousBuffer&& request,
                                               ::trpc::NoncontiguousBuffer& response) {
  TRPC_FMT_INFO("remote addr: {}:{}", context->GetIp(), context->GetPort());

  if (::trpc::IsRunningInFiberWorker()) {
    return FiberForward(context, std::move(request), response);
  }

  return FutureForward(context, std::move(request), response);

  /*::trpc::ClientContextPtr client_context = ::trpc::MakeTransparentClientContext(context, proxy_ptr_);
  client_context->SetTimeout(3000);

  ::trpc::Status status = proxy_ptr_->UnaryInvoke(client_context, request, &response);

  if (!status.OK()) {
    TRPC_FMT_ERROR("ret code: {}, func codec: {}, error msg: {}", status.GetFrameworkRetCode(),
                   status.GetFuncRetCode(), status.ErrorMessage());
  } else {
    TRPC_FMT_INFO("response size: {}", response.ByteSize());
  }

  return status;*/
}

::trpc::Status TransparentServiceImpl::FiberForward(::trpc::ServerContextPtr context,
                                                    ::trpc::NoncontiguousBuffer&& request,
                                                    ::trpc::NoncontiguousBuffer& response) {
  ::trpc::ClientContextPtr client_context = ::trpc::MakeTransparentClientContext(context, proxy_ptr_);
  client_context->SetTimeout(3000);

  ::trpc::Status status = proxy_ptr_->UnaryInvoke(client_context, request, &response);

  if (!status.OK()) {
    TRPC_FMT_ERROR("ret code: {}, func codec: {}, error msg: {}", status.GetFrameworkRetCode(),
                   status.GetFuncRetCode(), status.ErrorMessage());
  } else {
    TRPC_FMT_INFO("response size: {}", response.ByteSize());
  }

  return status;
}

::trpc::Status TransparentServiceImpl::FutureForward(::trpc::ServerContextPtr context,
                                                     ::trpc::NoncontiguousBuffer&& request,
                                                     ::trpc::NoncontiguousBuffer& response) {
  context->SetResponse(false);

  ::trpc::ClientContextPtr client_context = ::trpc::MakeTransparentClientContext(context, proxy_ptr_);
  client_context->SetTimeout(3000);

  proxy_ptr_->AsyncUnaryInvoke<::trpc::NoncontiguousBuffer, ::trpc::NoncontiguousBuffer>(client_context, request)
      .Then([context, this](::trpc::Future<::trpc::NoncontiguousBuffer>&& fut) mutable {
        trpc::Status status;
        trpc::NoncontiguousBuffer reply;

        if (fut.IsFailed()) {
          status.SetErrorMessage(fut.GetException().what());
          status.SetFuncRetCode(-1);
          TRPC_FMT_ERROR("error msg: {}", fut.GetException().what());

          std::string str(fut.GetException().what());
          reply = trpc::CreateBufferSlow(str.c_str());
        } else {
          reply = fut.GetValue0();

          TRPC_FMT_INFO("response size: {}", reply.ByteSize());
        }

        context->SendTransparentResponse(status, std::move(reply));
        return trpc::MakeReadyFuture<>();
      });

  return trpc::kSuccStatus;
}

}  // namespace examples::transparent
