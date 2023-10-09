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

#include <atomic>
#include <memory>
#include <string>
#include <utility>

#include "trpc/client/client_context.h"
#include "trpc/client/service_proxy.h"
#include "trpc/codec/protocol.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc {

/// @brief Non-rpc service proxy.
class NonRpcServiceProxy : public ServiceProxy {
 public:
  /// @brief Unary synchronous call, used for making calls with custom user protocol header+body (unserialized, the
  ///        protocol needs to be implemented and registered with the codec).
  template <class RequestProtocol, class ResponseProtocol>
  Status UnaryInvoke(const ClientContextPtr& context, const RequestProtocol& req, ResponseProtocol& rsp);

  /// @brief Unary asynchronous call with custom user protocol
  template <class RequestProtocol, class ResponseProtocol>
  Future<ResponseProtocol> AsyncUnaryInvoke(const ClientContextPtr& context, const RequestProtocol& req);

  /// @brief One way call with custom user protocol.
  template <class RequestProtocol>
  Status OnewayInvoke(const ClientContextPtr& context, const RequestProtocol& req);

 private:
  template <class RequestProtocol>
  void SetRequest(const ClientContextPtr& context, const RequestProtocol& req) {
    // Set req parameter to context. If req parameter is equal to req in context, it will do nothing.
    const ProtocolPtr& req_protocol = context->GetRequest();
    if (req_protocol == nullptr) {
      context->SetRequest(req);
    } else if (TRPC_UNLIKELY(req_protocol.get() != req.get())) {
      // If the req in the context isn't empty and is not equal to the input parameter, print a log message indicating
      // that it has been overwritten
      TRPC_FMT_WARN_EVERY_SECOND("`req` will replace of `request of context`");
      context->SetRequest(req);
    }
  }
};

template <class RequestProtocol, class ResponseProtocol>
Status NonRpcServiceProxy::UnaryInvoke(const ClientContextPtr& context, const RequestProtocol& req,
                                       ResponseProtocol& rsp) {
  const bool is_protocol_req = std::is_convertible_v<typename RequestProtocol::element_type*, Protocol*>;
  const bool is_protocol_rsp = std::is_convertible_v<typename ResponseProtocol::element_type*, Protocol*>;

  static_assert(is_protocol_req, "RequestProtocol::element_type is not derive of 'Protocol'");
  static_assert(is_protocol_rsp, "ResponseProtocol::element_type is not derive of 'Protocol'");

  SetRequest(context, req);

  FillClientContext(context);

  auto filter_ret = RunFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_ret == 0) {
    ProtocolPtr& rsp_protocol = context->GetResponse();
    Status unary_invoke_status = ServiceProxy::UnaryInvoke(context, req, rsp_protocol);
    if (unary_invoke_status.OK()) {
      rsp = std::dynamic_pointer_cast<typename ResponseProtocol::element_type>(rsp_protocol);
    }
  }

  RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);

  context->SetEndTimestampUs(trpc::time::GetMicroSeconds());

  return context->GetStatus();
}

template <class RequestProtocol, class ResponseProtocol>
Future<ResponseProtocol> NonRpcServiceProxy::AsyncUnaryInvoke(const ClientContextPtr& context,
                                                              const RequestProtocol& req) {
  const bool is_protocol_req = std::is_convertible_v<typename RequestProtocol::element_type*, Protocol*>;
  const bool is_protocol_rsp = std::is_convertible_v<typename ResponseProtocol::element_type*, Protocol*>;

  static_assert(is_protocol_req, "RequestProtocol::element_type is not derive of 'Protocol'");
  static_assert(is_protocol_rsp, "ResponseProtocol::element_type is not derive of 'Protocol'");

  SetRequest(context, req);

  FillClientContext(context);

  auto filter_ret = RunFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_ret != 0) {
    TRPC_FMT_ERROR("service name:{}, filter execute failed.", GetServiceName());
    RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);

    context->SetEndTimestampUs(trpc::time::GetMicroSeconds());

    const Status& result = context->GetStatus();
    return MakeExceptionFuture<ResponseProtocol>(CommonException(result.ErrorMessage().c_str()));
  }

  return ServiceProxy::AsyncUnaryInvoke(context, req).Then([this, context](Future<ProtocolPtr>&& rsp) {
    RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);

    context->SetEndTimestampUs(trpc::time::GetMicroSeconds());

    if (rsp.IsReady()) {
      ProtocolPtr result = rsp.GetValue0();
      ResponseProtocol rsp_data = std::dynamic_pointer_cast<typename ResponseProtocol::element_type>(result);
      return MakeReadyFuture<ResponseProtocol>(std::move(rsp_data));
    } else {
      return MakeExceptionFuture<ResponseProtocol>(rsp.GetException());
    }
  });
}

template <class RequestProtocol>
Status NonRpcServiceProxy::OnewayInvoke(const ClientContextPtr& context, const RequestProtocol& req) {
  const bool is_protocol_req = std::is_convertible_v<RequestProtocol, ProtocolPtr>;

  static_assert(is_protocol_req, "RequestProtocol is not derive of 'ProtocolPtr'");

  SetRequest(context, req);

  FillClientContext(context);

  context->SetCallType(kOnewayCall);

  auto filter_ret = RunFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_ret == 0) {
    ServiceProxy::OnewayInvoke(context, req);
  }
  RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);

  context->SetEndTimestampUs(trpc::time::GetMicroSeconds());

  return context->GetStatus();
}

}  // namespace trpc
