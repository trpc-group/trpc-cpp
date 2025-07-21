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

#include "trpc/server/http_service.h"

#include "trpc/codec/http/http_protocol.h"
#include "trpc/util/deferred.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc {

void HttpService::HandleTransportMessage(STransportReqMsg* recv, STransportRspMsg** send) noexcept {
  ServerContextPtr& context = recv->context;
  http::RequestPtr& req =
      static_cast<HttpRequestProtocol*>(context->GetRequestMsg().get())->request;  // NOLINT: safe unchecked cast
  http::Response& rsp =
      static_cast<HttpResponseProtocol*>(context->GetResponseMsg().get())->response;  // NOLINT: safe unchecked cast
  rsp.SetHeaderOnly(req->GetMethodType() == http::HEAD);

  context->SetRequestData(req.get());
  context->SetResponseData(&rsp);
  // Sets "EncodeType", other plugins may need it.
  context->SetReqEncodeType(MimeToEncodeType(req->GetHeader(http::kHeaderContentType)));
  context->SetFuncName(req->GetRouteUrl());
  context->SetRealTimeout();
  auto& req_stream = req->GetStream();
  req_stream.SetDefaultDeadline(trpc::ReadSteadyClock() + std::chrono::milliseconds(context->GetTimeout()));
  std::string uri_path = trpc::http::NormalizeUrl(req->GetRouteUrlView());

  http::HandlerBase* handler = routes_.GetHandler(uri_path, req);
  if (!handler || !handler->IsStream()) {  // path not found or non-stream handler
    Status status = req_stream.AppendToRequest(req->GetMaxBodySize());
    if (status.OK()) {
      Handle(uri_path, handler, context, req, rsp, send);
    } else {
      HandleError(context, req, rsp, status);
    }
  } else {  // stream handler
    rsp.EnableStream(context.get());
    Handle(uri_path, handler, context, req, rsp, send);
    rsp.GetStream().Close();
    // Closes the connection if stream was broken.
    if (context->GetStatus().StreamRst()) {
      TRPC_FMT_TRACE("{} {}, error: {}", req->GetMethod(), req->GetRouteUrlView(), context->GetStatus().ToString());
      context->CloseConnection();
    }
  }
  // Release request stream.
  req_stream.Close();
}

Status HttpService::Dispatch(const std::string& path, http::HandlerBase* handler, ServerContextPtr& context,
                             http::RequestPtr& req, http::Response& rsp) {
  return routes_.Handle(path, handler, context, req, rsp);
}

void HttpService::Handle(const std::string& path, http::HandlerBase* handler, ServerContextPtr& context,
                         http::RequestPtr& req, http::Response& rsp, STransportRspMsg** send) {
  Deferred _([&context]() {
    context->SetRequestData(nullptr);
    context->SetResponseData(nullptr);
  });

  // For some monitor plugins.
  auto msg_filter_status = GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_POST_RECV_MSG, context);
  // For some tracing or log replay plugins.
  auto rpc_filter_status = GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_PRE_RPC_INVOKE, context);

  // Rejected by filters.
  if (TRPC_UNLIKELY(msg_filter_status == FilterStatus::REJECT || rpc_filter_status == FilterStatus::REJECT)) {
    // For some tracing or log replay plugins.
    GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_POST_RPC_INVOKE, context);
    // For some monitor plugins.
    GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_PRE_SEND_MSG, context);

    auto& status = context->GetStatus();
    status.SetFrameworkRetCode(GetDefaultServerRetCode(codec::ServerRetCode::INVOKE_UNKNOW_ERROR));
    status.SetErrorMessage("filter reject");
    http::Response reject_rsp;
    *send = trpc::object_pool::New<STransportRspMsg>();
    (*send)->context = context;
    reject_rsp.GenerateExceptionReply(http::ResponseStatus::kForbidden, req->GetVersion(), "request reject");
    std::move(reject_rsp).SerializeToString((*send)->buffer);
    return;
  }

  // Timeout.
  CheckTimeout(context);
  if (!context->GetStatus().OK()) {
    // For some tracing or log replay plugins.
    GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_POST_RPC_INVOKE, context);
    // For some monitor plugins.
    GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_PRE_SEND_MSG, context);

    TRPC_LOG_ERROR("CheckTimeout failed, ip: " << context->GetIp()
                                               << ", service queue_timeout: " << GetServiceAdapterOption().queue_timeout
                                               << ", client timeout:" << context->GetTimeout());

    auto& status = context->GetStatus();
    status.SetFrameworkRetCode(GetDefaultServerRetCode(codec::ServerRetCode::TIMEOUT_ERROR));
    status.SetErrorMessage("client timeout");
    http::Response timeout_rsp;
    static const std::string request_timeout_ex = http::JsonException(http::RequestTimeout()).ToJson();
    timeout_rsp.GenerateExceptionReply(http::ResponseStatus::kGatewayTimeout, req->GetVersion(), request_timeout_ex);
    NoncontiguousBuffer buffer;
    std::move(timeout_rsp).SerializeToString(buffer);
    context->SendResponse(std::move(buffer));
    context->CloseConnection();
    return;
  }

  // Runs user handler.
  auto status = Dispatch(path, handler, context, req, rsp);
  if (context->IsResponse()) {
    context->SetStatus(std::move(status));
    // For some tracing or log replay plugins.
    GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_POST_RPC_INVOKE, context);
    // For some monitor plugins.
    GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_PRE_SEND_MSG, context);

    *send = trpc::object_pool::New<STransportRspMsg>();
    (*send)->context = context;

    if (context->CheckHandleTimeout()) {
      http::Response timeout_rsp;
      static const std::string request_handle_timeout_ex =
          http::JsonException(http::RequestTimeout("Request Handle Timeout")).ToJson();
      timeout_rsp.GenerateExceptionReply(http::ResponseStatus::kGatewayTimeout, req->GetVersion(),
                                         request_handle_timeout_ex);
      std::move(timeout_rsp).SerializeToString((*send)->buffer);
    } else {
      std::move(rsp).SerializeToString((*send)->buffer);
    }
  }
}

void HttpService::HandleError(ServerContextPtr& context, http::RequestPtr& req, http::Response& rsp,
                              const Status& status) {
  req->GetStream().Close(stream::HttpReadStream::ReadState::kErrorBit);
  if (status.GetFrameworkRetCode() == stream::kStreamStatusServerReadTimeout.GetFrameworkRetCode()) {
    rsp.GenerateExceptionReply(http::ResponseStatus::kRequestTimeout, req->GetVersion(), "request timeout");
  } else if (status.GetFrameworkRetCode() == stream::kStreamStatusServerMessageExceedLimit.GetFrameworkRetCode()) {
    rsp.GenerateExceptionReply(http::ResponseStatus::kRequestEntityTooLarge, req->GetVersion(),
                               "request entity too large");
  } else {  // unexpected error
    rsp.GenerateExceptionReply(http::ResponseStatus::kInternalServerError, req->GetVersion(), "unknown error");
  }
  TRPC_LOG_DEBUG("HTTP read error, ip: " << context->GetIp() << ", status: " << status.ToString());
  NoncontiguousBuffer buffer;
  std::move(rsp).SerializeToString(buffer);
  context->SendResponse(std::move(buffer));
  context->SetRequestData(nullptr);
  context->SetResponseData(nullptr);
  context->CloseConnection();
}

void HttpService::CheckTimeout(const ServerContextPtr& context) {
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

}  // namespace trpc
