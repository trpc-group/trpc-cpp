// trpc/server/http_sse/http_sse_server.cc
//
// SSE-specialized HTTP service based on HttpService::HandleTransportMessage.
// Minimal SSE-specific modifications: detect SSE requests and enable SSE headers + streaming.
// Other logic (filters, timeout, dispatch, error handling) copied from HttpService.
//

#include "trpc/server/http_sse/http_sse_service.h"
#include "trpc/codec/http/http_protocol.h"
#include "trpc/codec/http_sse/http_sse_proto_checker.h"  // IsValidSseRequest
#include "trpc/util/deferred.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc {

void HttpSseService::HandleTransportMessage(STransportReqMsg* recv, STransportRspMsg** send) noexcept {
  ServerContextPtr& context = recv->context;

  // request / response protocol objects (same as HttpService)
  http::RequestPtr& req =
      static_cast<HttpRequestProtocol*>(context->GetRequestMsg().get())->request;  // NOLINT: safe unchecked cast
  http::Response& rsp =
      static_cast<HttpResponseProtocol*>(context->GetResponseMsg().get())->response;  // NOLINT: safe unchecked cast
  rsp.SetHeaderOnly(req->GetMethodType() == http::HEAD);

  // attach request/response pointers to context for filters/handlers
  context->SetRequestData(req.get());
  context->SetResponseData(&rsp);

  // set encode-type and function name, and compute real timeout
  context->SetReqEncodeType(MimeToEncodeType(req->GetHeader(http::kHeaderContentType)));
  context->SetFuncName(req->GetRouteUrl());
  context->SetRealTimeout();

  // configure request stream default deadline (for blocking reads)
  auto& req_stream = req->GetStream();
  req_stream.SetDefaultDeadline(trpc::ReadSteadyClock() + std::chrono::milliseconds(context->GetTimeout()));

  // route lookup
  std::string uri_path = trpc::http::NormalizeUrl(req->GetRouteUrlView());
  http::HandlerBase* handler = routes_.GetHandler(uri_path, req);

  // If the handler is a stream handler, check SSE specifics and handle accordingly
  if (handler && handler->IsStream()) {
    // detect SSE request: Accept contains "text/event-stream" and method is GET
    bool is_sse = IsValidSseRequest(req.get());

    if (is_sse) {
      TRPC_LOG_INFO("HttpSseService: detected SSE request for path=" << uri_path);

      // Enable streaming on response
      rsp.EnableStream(context.get());

      // Pre-set SSE headers on the response so handler sees them by default
      // These are the canonical SSE headers. Handler may override if needed.
      rsp.SetMimeType("text/event-stream");
      rsp.SetHeader("Cache-Control", "no-cache");
      rsp.SetHeader("Connection", "keep-alive");
      // allow CORS by default (matches codec helper earlier)
      //rsp.SetHeader("Access-Control-Allow-Origin", "*");
      //rsp.SetHeader("Access-Control-Allow-Headers", "Cache-Control");
      // use chunked transfer encoding for streaming responses
      rsp.SetHeader(http::kHeaderTransferEncoding, http::kTransferEncodingChunked);

      // Call handler (streaming mode). Handler should write events via rsp.GetStream() or use your SseStreamWriter.
      Handle(uri_path, handler, context, req, rsp, send);

      // After handler returns, close stream and check possible stream-reset
      try {
        rsp.GetStream().Close();
      } catch (...) {
      }

      if (context->GetStatus().StreamRst()) {
        TRPC_FMT_TRACE("{} {}, error: {}", req->GetMethod(), req->GetRouteUrlView(), context->GetStatus().ToString());
        context->CloseConnection();
      }

      // Release request stream
      req_stream.Close();
      return;
    }
    // else: it's a stream handler but not SSE -> fall through to normal stream handling below
  }

  // Non-stream or non-SSE stream: follow original HttpService behavior
  if (!handler || !handler->IsStream()) {
    Status status = req_stream.AppendToRequest(req->GetMaxBodySize());
    if (status.OK()) {
      Handle(uri_path, handler, context, req, rsp, send);
    } else {
      HandleError(context, req, rsp, status);
    }
  } else {  // stream handler but not SSE
    rsp.EnableStream(context.get());
    Handle(uri_path, handler, context, req, rsp, send);
    rsp.GetStream().Close();
    if (context->GetStatus().StreamRst()) {
      TRPC_FMT_TRACE("{} {}, error: {}", req->GetMethod(), req->GetRouteUrlView(), context->GetStatus().ToString());
      context->CloseConnection();
    }
  }

  // Release request stream
  req_stream.Close();
}

Status HttpSseService::Dispatch(const std::string& path, http::HandlerBase* handler, ServerContextPtr& context,
                                http::RequestPtr& req, http::Response& rsp) {
  return routes_.Handle(path, handler, context, req, rsp);
}

void HttpSseService::Handle(const std::string& path, http::HandlerBase* handler, ServerContextPtr& context,
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

void HttpSseService::HandleError(ServerContextPtr& context, http::RequestPtr& req, http::Response& rsp,
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

void HttpSseService::CheckTimeout(const ServerContextPtr& context) {
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

