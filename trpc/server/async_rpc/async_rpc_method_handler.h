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

#include "trpc/future/future.h"
#include "trpc/server/rpc/unary_rpc_method_handler.h"
#include "trpc/util/log/logging.h"

namespace trpc {

/// @brief The implementation of asynchronous rpc method handler.
template <class RequestType, class ResponseType>
class AsyncRpcMethodHandler : public UnaryRpcMethodHandler<RequestType, ResponseType> {
 public:
  using RpcAsyncMethodFunction = std::function<Future<ResponseType>(const ServerContextPtr&, const RequestType* req)>;

  using UnaryRpcMethodHandler<RequestType, ResponseType>::PreExecute;
  using UnaryRpcMethodHandler<RequestType, ResponseType>::PostExecute;

  explicit AsyncRpcMethodHandler(const RpcAsyncMethodFunction& func) : func_(func) {}

  void Execute(const ServerContextPtr& context) noexcept override {
    if (!PreExecute(context, context->GetRequestMsg()->GetNonContiguousProtocolBody())) {
      NoncontiguousBuffer rsp_body;
      if (!IsDecodeError(context)) {
        PostExecute(context, rsp_body);
      }
      context->GetResponseMsg()->SetNonContiguousProtocolBody(std::move(rsp_body));
      return;
    }

    context->SetResponse(false);

    auto req = static_cast<RequestType*>(context->GetRequestData());
    func_(context, req)
        .Then([context](ResponseType&& rsp) {
          context->SendUnaryResponse(kDefaultStatus, rsp);
          return MakeReadyFuture<>();
        })
        .Then([this, context](Future<>&& ft) {
          if (ft.IsFailed()) {
            auto ex = ft.GetException();
            TRPC_FMT_ERROR("Unary rpc failed with error: {}", ex.what());
            context->SendUnaryResponse(ExceptionToStatus(std::move(ex)));
          }
          return MakeReadyFuture<>();
        });  // future discarded
  }

 private:
  Status ExceptionToStatus(Exception&& ex) {
    if (ex.is<UnaryRpcError>()) {
      return ex.Get<UnaryRpcError>()->GetStatus();
    } else {
      return Status(ex.GetExceptionCode(), 0, ex.what());
    }
  }

  void Execute(const ServerContextPtr& context, NoncontiguousBuffer&& req_body,
               NoncontiguousBuffer& rsp_body) noexcept override {
    TRPC_ASSERT(false && "Unreachable");
  }

  bool IsDecodeError(const ServerContextPtr& context) {
    return context->GetStatus().GetFrameworkRetCode() ==
           context->GetServerCodec()->GetProtocolRetCode(codec::ServerRetCode::DECODE_ERROR);
  }

 private:
  RpcAsyncMethodFunction func_;
};

}  // namespace trpc
