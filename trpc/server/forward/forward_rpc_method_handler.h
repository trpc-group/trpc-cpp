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

#pragma once

#include <functional>
#include <string>
#include <utility>

#include "trpc/server/method_handler.h"
#include "trpc/server/server_context.h"
#include "trpc/util/log/logging.h"

namespace trpc {

/// @brief The implementation of rpc transparent transmission method handler
class ForwardRpcMethodHandler : public RpcMethodHandlerInterface {
 public:
  using ForwardRpcFunction =
      std::function<Status(ServerContextPtr, NoncontiguousBuffer&&, NoncontiguousBuffer&)>;

  explicit ForwardRpcMethodHandler(const ForwardRpcFunction& func) : forward_func_(func) {}

  void Execute(const ServerContextPtr& context, NoncontiguousBuffer&& req_body,
               NoncontiguousBuffer& rsp_body) noexcept override {
    uint32_t encode_type = context->GetReqEncodeType();

    TRPC_LOG_TRACE("encode_type:" << encode_type);

    Status status = forward_func_(context, std::move(req_body), rsp_body);

    if (context->IsResponse()) {
      if (!status.OK()) {
        context->SetStatus(std::move(status));
      }
    }
  }

  void Execute(const ServerContextPtr& context) noexcept override { TRPC_ASSERT(false && "Unreachable"); }

 private:
  ForwardRpcFunction forward_func_;
};

}  // namespace trpc
