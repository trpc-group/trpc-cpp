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

#include "trpc/server/non_rpc/non_rpc_service_impl.h"

#include <string>
#include <utility>

#include "trpc/codec/codec_helper.h"
#include "trpc/server/server_context.h"
#include "trpc/util/log/logging.h"

namespace trpc {

void NonRpcServiceImpl::Dispatch(const ServerContextPtr& context, const ProtocolPtr& req,
                                 ProtocolPtr& rsp)  noexcept {
  auto& non_rpc_service_methods = GetNonRpcServiceMethod();
  auto it = non_rpc_service_methods.find(context->GetFuncName());
  if (it == non_rpc_service_methods.end()) {
    HandleNoFuncError(context);

    return;
  }

  NonRpcMethodHandlerInterface *method_handler = it->second->GetNonRpcMethodHandler();

  method_handler->Execute(context, req, rsp);
}

}  // namespace trpc
