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

#include "trpc/server/rpc/unary_rpc_method_handler.h"

namespace trpc {

/// @brief The implementation of rpc method handler
template <class RequestType, class ResponseType>
class RpcMethodHandler : public UnaryRpcMethodHandler<RequestType, ResponseType> {
 public:
  using RpcMethodFunction = std::function<Status(ServerContextPtr, const RequestType*, ResponseType*)>;

  using UnaryRpcMethodHandler<RequestType, ResponseType>::PreExecute;
  using UnaryRpcMethodHandler<RequestType, ResponseType>::PostExecute;

  explicit RpcMethodHandler(const RpcMethodFunction& func) : func_(func) {}

  void Execute(const ServerContextPtr& context, NoncontiguousBuffer&& req_body,
               NoncontiguousBuffer& rsp_body) noexcept override {
    if (PreExecute(context, std::move(req_body))) {
      auto status = func_(context, static_cast<RequestType*>(context->GetRequestData()),
                          static_cast<ResponseType*>(context->GetResponseData()));

      if (context->IsResponse()) {
        context->SetStatus(std::move(status));
      } else {
        // if the user calls context->SetResponse(false),
        // the user will use context->SendUnaryResponse to return the package asynchronously,
        // and return directly here
        return;
      }
    } else if (IsDecodeError(context)) {
      // if decoding error, no need to execute PostExecute
      return;
    }

    PostExecute(context, rsp_body);
  }

 private:
  void Execute(const ServerContextPtr& context) noexcept override { TRPC_ASSERT(false && "Unreachable"); }
  bool IsDecodeError(const ServerContextPtr& context) {
    return context->GetStatus().GetFrameworkRetCode() ==
           context->GetServerCodec()->GetProtocolRetCode(codec::ServerRetCode::DECODE_ERROR);
  }

 private:
  RpcMethodFunction func_;
};

}  // namespace trpc
