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

#include "trpc/codec/protocol.h"
#include "trpc/server/service.h"

namespace trpc {

/// @brief Service implement
class ServiceImpl : public Service {
 public:
  void HandleTransportMessage(STransportReqMsg* recv, STransportRspMsg** send) noexcept override;
  void HandleTransportStreamMessage(STransportReqMsg* recv) noexcept override;

  /// @brief The method for dispatching and processing unary request
  /// @param context Request context
  /// @param [in] req Request protocol
  /// @param [out] rsp Response protocol
  virtual void Dispatch(const ServerContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp) noexcept {
    TRPC_ASSERT(false && "not implemented.");
  }

  /// @brief The method for dispatching and processing stream request
  /// @param context Request context
  virtual void DispatchStream(const ServerContextPtr& context) noexcept {
    TRPC_ASSERT(false && "not implemented.");
  }

 protected:
  RpcMethodHandlerInterface* GetUnaryRpcMethodHandler(const std::string& func_name);
  RpcMethodHandlerInterface* GetStreamRpcMethodHandler(const std::string& func_name);
  void HandleNoFuncError(const ServerContextPtr& context);
  void CheckTimeoutBeforeProcess(const ServerContextPtr& context);
  void ConstructUnaryResponse(const ServerContextPtr& context, ProtocolPtr& rsp, STransportRspMsg** send);
  void HandleTimeout(const ServerContextPtr& context);
};

}  // namespace trpc
