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
#include <string>
#include <utility>

#include "trpc/codec/codec_helper.h"
#include "trpc/server/method_handler.h"
#include "trpc/server/server_context.h"
#include "trpc/util/log/logging.h"

namespace trpc {

/// @brief The implementation of non-rpc method handler
/// @note  The concrete types of the template types
//         `RequestType` and `ResponseType` are `Protocol` subclasses
template <class RequestType, class ResponseType>
class NonRpcMethodHandler : public NonRpcMethodHandlerInterface {
 public:
  using NonRpcMethodFunction = std::function<Status(ServerContextPtr, const RequestType*, ResponseType*)>;
  using NonRpcMethodSmartPtrFunction = std::function<Status(
      ServerContextPtr, const std::shared_ptr<RequestType>& req, std::shared_ptr<ResponseType>& rsp)>;

  explicit NonRpcMethodHandler(const NonRpcMethodFunction& func) : func_(func) {}

  explicit NonRpcMethodHandler(const NonRpcMethodSmartPtrFunction& func) : smart_ptr_func_(func) {}

  void Execute(const ServerContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp) noexcept override {
    Status status;
    if (smart_ptr_func_) {
      std::shared_ptr<RequestType> req_ptr = dynamic_pointer_cast<RequestType>(req);
      std::shared_ptr<ResponseType> rsp_ptr = dynamic_pointer_cast<ResponseType>(rsp);
      status = smart_ptr_func_(context, req_ptr, rsp_ptr);
    } else {
      const RequestType* request = dynamic_cast<const RequestType*>(req.get());
      ResponseType* response = dynamic_cast<ResponseType*>(rsp.get());
      status = func_(context, request, response);
    }

    if (context->IsResponse()) {
      context->SetStatus(std::move(status));
    }

    return;
  }

 private:
  NonRpcMethodFunction func_;
  NonRpcMethodSmartPtrFunction smart_ptr_func_;
};

}  // namespace trpc
