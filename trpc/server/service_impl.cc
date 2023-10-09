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

#include "trpc/server/service_impl.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "trpc/codec/codec_helper.h"
#include "trpc/server/server_context.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc {

void ServiceImpl::HandleTransportMessage(STransportReqMsg* recv, STransportRspMsg** send) noexcept {
  auto& filter_controller = GetFilterController();
  ServerContextPtr& context = recv->context;
  auto& status = context->GetStatus();
  if (!status.OK() || !context->GetRequestMsg()->WaitForFullRequest()) {
    // if decode fails, and decode failure does not require a return packet
    // then return directly
    // note: trpc protocol does not return a packet when decoding fails
    //       http protocol does return a packet when decoding fails
    if (status.GetFrameworkRetCode() == GetDefaultServerRetCode(codec::ServerRetCode::DECODE_ERROR)) {
      if (!context->NeedResponseWhenDecodeFail()) {
        return;
      }
    }

    filter_controller.RunMessageServerFilters(FilterPoint::SERVER_POST_RECV_MSG, context);
    filter_controller.RunMessageServerFilters(FilterPoint::SERVER_PRE_SEND_MSG, context);

    ConstructUnaryResponse(context, context->GetResponseMsg(), send);

    return;
  }

  // set the actual timeout time of the current request
  // take the minimum value between the link timeout time and
  // the timeout time configured by the service
  context->SetRealTimeout();

  auto filter_status = filter_controller.RunMessageServerFilters(FilterPoint::SERVER_POST_RECV_MSG, context);
  if (filter_status == FilterStatus::REJECT) {
    filter_controller.RunMessageServerFilters(FilterPoint::SERVER_PRE_SEND_MSG, context);
    ConstructUnaryResponse(context, context->GetResponseMsg(), send);
    return;
  }

  // check whether the request has timed out due to reasons such as queuing before process
  CheckTimeoutBeforeProcess(context);
  if (!context->GetStatus().OK()) {
    HandleTimeout(context);

    TRPC_LOG_ERROR("CheckTimeoutBeforeProcess failed, ip: " << context->GetIp() << ", service queue_timeout: "
                                               << GetServiceAdapterOption().queue_timeout
                                               << ", client timeout:" << context->GetTimeout());

    filter_controller.RunMessageServerFilters(FilterPoint::SERVER_PRE_SEND_MSG, context);

    ConstructUnaryResponse(context, context->GetResponseMsg(), send);
    return;
  }

  Dispatch(context, context->GetRequestMsg(), context->GetResponseMsg());

  // the request of one-way call does not return a packet
  if (context->GetCallType() == kOnewayCall) {
    filter_controller.RunMessageServerFilters(FilterPoint::SERVER_PRE_SEND_MSG, context);
    return;
  }

  if (context->IsResponse()) {
    // check whether the request has timed out after processing
    context->CheckHandleTimeout();

    filter_controller.RunMessageServerFilters(FilterPoint::SERVER_PRE_SEND_MSG, context);

    ConstructUnaryResponse(context, context->GetResponseMsg(), send);
  }
}

void ServiceImpl::HandleTransportStreamMessage(STransportReqMsg* recv) noexcept {
  ServerContextPtr& context = recv->context;
  context->GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_POST_RECV_MSG, context);

  DispatchStream(context);
}

RpcMethodHandlerInterface* ServiceImpl::GetUnaryRpcMethodHandler(const std::string& func_name) {
  const auto& rpc_service_methods = GetRpcServiceMethod();
  auto it = rpc_service_methods.find(func_name);
  if (it != rpc_service_methods.end() && it->second->GetMethodType() == MethodType::UNARY) {
    return it->second->GetRpcMethodHandler();
  }

  TRPC_LOG_ERROR("service: " << GetName() << ", unary func:" << func_name << " not found.");

  return nullptr;
}

RpcMethodHandlerInterface* ServiceImpl::GetStreamRpcMethodHandler(const std::string& func_name) {
  const auto& rpc_service_methods = GetRpcServiceMethod();
  auto it = rpc_service_methods.find(func_name);
  if (it != rpc_service_methods.end() && it->second->GetMethodType() != MethodType::UNARY) {
    return it->second->GetRpcMethodHandler();
  }

  TRPC_LOG_ERROR("service: " << GetName() << ", stream func:" << func_name << " not found.");

  return nullptr;
}

void ServiceImpl::HandleNoFuncError(const ServerContextPtr& context) {
  TRPC_LOG_ERROR("func:" << context->GetFuncName() << " not found");

  std::string err_msg = context->GetFuncName();
  err_msg += " not found";

  TRPC_ASSERT(context->GetServerCodec() != nullptr);

  context->GetStatus().SetFrameworkRetCode(
      context->GetServerCodec()->GetProtocolRetCode(trpc::codec::ServerRetCode::NOT_FUN_ERROR));
  context->GetStatus().SetErrorMessage(std::move(err_msg));
}

void ServiceImpl::CheckTimeoutBeforeProcess(const ServerContextPtr& context) {
  uint64_t now_ms = static_cast<int64_t>(trpc::time::GetMilliSeconds());
  uint64_t timeout = std::min(GetServiceAdapterOption().queue_timeout, context->GetTimeout());

  if (context->GetRecvTimestamp() + timeout <= now_ms) {
    bool use_queue_timeout = GetServiceAdapterOption().queue_timeout < context->GetTimeout();
    if (!use_queue_timeout && context->IsUseFullLinkTimeout()) {
      context->GetStatus().SetFrameworkRetCode(
          context->GetServerCodec()->GetProtocolRetCode(trpc::codec::ServerRetCode::FULL_LINK_TIMEOUT_ERROR));
      context->GetStatus().SetErrorMessage("request full-link timeout.");
    } else {
      context->GetStatus().SetFrameworkRetCode(
          context->GetServerCodec()->GetProtocolRetCode(trpc::codec::ServerRetCode::TIMEOUT_ERROR));
      context->GetStatus().SetErrorMessage("request timeout.");
    }
  }
}

void ServiceImpl::HandleTimeout(const ServerContextPtr& context) {
  ServiceTimeoutHandleFunction& timeout_handler = GetServiceTimeoutHandleFunction();
  if (timeout_handler) {
    timeout_handler(context);
  }
}

void ServiceImpl::ConstructUnaryResponse(const ServerContextPtr& context, ProtocolPtr& rsp, STransportRspMsg** send) {
  if (context->GetCallType() == kOnewayCall) {
    return;
  }

  NoncontiguousBuffer send_data;

  if (!context->GetServerCodec()->ZeroCopyEncode(context, context->GetResponseMsg(), send_data)) {
    TRPC_FMT_ERROR("request encode failed, ip:{}, port:{}", context->GetIp(), context->GetPort());
    return;
  }

  *send = trpc::object_pool::New<STransportRspMsg>();
  (*send)->context = context;
  (*send)->buffer = std::move(send_data);
}

}  // namespace trpc
