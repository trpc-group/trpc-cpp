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

#include <memory>

#include "trpc/codec/server_codec.h"
#include "trpc/server/non_rpc/non_rpc_method_handler.h"
#include "trpc/server/non_rpc/non_rpc_service_impl.h"

namespace trpc {

/// @brief Encapsulated non-rpc request processing, convenient for users
template <class RequestType, class ResponseType>
class SimpleNonRpcServiceImpl : public NonRpcServiceImpl {
 public:
  SimpleNonRpcServiceImpl() {
    auto handler = new trpc::NonRpcMethodHandler<RequestType, ResponseType>(std::bind(
        &SimpleNonRpcServiceImpl::Execute, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    AddNonRpcServiceMethod(new trpc::NonRpcServiceMethod(trpc::kNonRpcName, trpc::MethodType::UNARY, handler));
  }

  /// @brief Execute function, the user needs to inherit
  /// @param context request context
  /// @param in request protocol pointer
  /// @param out response protocol pointer
  /// @return Status execute result
  virtual trpc::Status Execute(const trpc::ServerContextPtr& context, const RequestType* in, ResponseType* out) = 0;
};

}  // namespace trpc
