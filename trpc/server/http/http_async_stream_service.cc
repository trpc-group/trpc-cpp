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

#include "trpc/server/http/http_async_stream_service.h"

#include "trpc/codec/http/http_protocol.h"
#include "trpc/stream/http/async/stream_reader_writer.h"
#include "trpc/util/http/common.h"
#include "trpc/util/http/util.h"
#include "trpc/util/log/logging.h"

namespace trpc::stream {

namespace {
std::string_view GetRouteUrl(const std::string& url) {
  return std::string_view(url).substr(0, std::min(url.find('?'), url.length()));
}
}  // namespace

void HttpAsyncStreamService::SendResponse(const ServerContextPtr& context, http::HttpResponse&& rsp) {
  NoncontiguousBuffer buffer;
  std::move(rsp).SerializeToString(buffer);
  STransportRspMsg* send = trpc::object_pool::New<STransportRspMsg>();
  send->context = context;
  send->buffer = std::move(buffer);
  SendMsg(send);
}

void HttpAsyncStreamService::HandleError(const ServerContextPtr& context, Status&& err_status,
                                         const HttpServerAsyncStreamPtr& stream) {
  http::HttpResponse rsp;
  rsp.GenerateExceptionReply(err_status.GetFrameworkRetCode(), stream->GetRequestLine().version,
                             err_status.ErrorMessage());
  context->SetStatus(std::move(err_status));
  GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_PRE_SEND_MSG, context);
  SendResponse(context, std::move(rsp));
  stream->Reset();
}

void HttpAsyncStreamService::DispatchStream(const ServerContextPtr& context) noexcept {
  auto stream = static_pointer_cast<HttpServerAsyncStream>(context->GetStreamReaderWriterProvider());
  TRPC_ASSERT(stream && "Unexpected nullptr stream");

  // check whether the request has timed out before handling request
  CheckTimeoutBeforeProcess(context);
  if (!context->GetStatus().OK()) {
    TRPC_FMT_ERROR("Gateway CheckTimeout failed, ip: {}, service queue_timeout: {}, timeout: {}", context->GetIp(),
                   GetServiceAdapterOption().queue_timeout, context->GetTimeout());
    return HandleError(context, Status{http::ResponseStatus::kGatewayTimeout, 0, "gateway timeout"}, stream);
  }

  const HttpRequestLine& line = stream->GetRequestLine();
  std::string_view route_url = GetRouteUrl(line.request_uri);
  std::string http_path = http::NormalizeUrl(route_url);
  http::OperationType http_method = http::StringToType(line.method);
  http::HandlerBase* handler = routes_.GetHandler(http_method, http_path, *stream->GetMutableParameters());

  // the method to handle this request does not exist.
  if (handler == nullptr) {
    TRPC_FMT_ERROR("http method or path not found, method: {}, path: {}", line.method, route_url);
    return HandleError(context, Status{http::ResponseStatus::kNotFound, 0, "not found"}, stream);
  }

  if (handler->IsStream()) {
    HandleStreamMethod(context, stream, handler);
  } else {
    HandleUnaryMethod(context, stream, handler);
  }
}

void HttpAsyncStreamService::HandleUnaryMethodImpl(const ServerContextPtr& context,
                                                   const HttpServerAsyncStreamPtr& stream,
                                                   http::FuncHandler* func_handler, http::HttpRequestPtr& req) {
  // executes the filters before rpc methods called
  FilterStatus filter_status =
      GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_PRE_RPC_INVOKE, context);
  if (TRPC_UNLIKELY(filter_status == FilterStatus::REJECT)) {
    GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_POST_RPC_INVOKE, context);
    return HandleError(context, Status{http::ResponseStatus::kForbidden, 0, "request reject"}, stream);
  }
  http::Response& rsp = static_cast<HttpResponseProtocol*>(context->GetResponseMsg().get())->response;
  routes_.Handle("", func_handler, context, req, rsp);
  if (context->IsResponse()) {
    // executes the filters after rpc methods called
    GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_POST_RPC_INVOKE, context);
    if (context->CheckHandleTimeout()) {
      TRPC_FMT_ERROR("Request HandleTimeout, ip: {}, timeout: {}", context->GetIp(), context->GetTimeout());
      return HandleError(context, Status{http::ResponseStatus::kRequestTimeout, 0, "request handle timeout"}, stream);
    }
    GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_PRE_SEND_MSG, context);
    SendResponse(context, std::move(rsp));
  }
  stream->Stop();
}

void HttpAsyncStreamService::HandleUnaryMethod(const ServerContextPtr& context, const HttpServerAsyncStreamPtr& stream,
                                               http::HandlerBase* handler) {
  auto func_handler = static_cast<http::FuncHandler*>(handler);
  // If the full request packet is received directly, it can be distributed to the user method for better performance.
  // If it is not received in full, it will wait for the full packet to be read before distributing it.
  if (stream->HasFullMessage()) {
    http::HttpRequestPtr req = stream->GetFullRequest();
    static_cast<HttpRequestProtocol*>(context->GetRequestMsg().get())->request = req;
    return HandleUnaryMethodImpl(context, stream, func_handler, req);
  }
  auto rw = MakeRefCounted<HttpServerAsyncStreamReaderWriter>(stream);
  ReadFullRequest(rw, context->GetTimeout())
      .Then([this, context, stream, func_handler](Future<http::HttpRequestPtr>&& ft) {
        if (ft.IsFailed()) {
          Exception ex = ft.GetException();
          Status err_status;
          if (ex.is<Timeout>()) {
            TRPC_FMT_ERROR("Request CheckTimeout failed, ip: {}, timeout: {}", context->GetIp(), context->GetTimeout());
            err_status.SetFrameworkRetCode(http::ResponseStatus::kRequestTimeout);
            err_status.SetErrorMessage("request wait timeout");
          } else {
            // other internal errors within the stream are treated as unknown errors
            TRPC_FMT_ERROR(ex.what());
            err_status.SetFrameworkRetCode(http::ResponseStatus::kInternalServerError);
            err_status.SetErrorMessage("internal error");
          }
          return HandleError(context, std::move(err_status), stream);
        }
        http::HttpRequestPtr req = ft.GetValue0();
        static_cast<HttpRequestProtocol*>(context->GetRequestMsg().get())->request = req;
        return HandleUnaryMethodImpl(context, stream, func_handler, req);
      });
}

void HttpAsyncStreamService::HandleStreamMethod(const ServerContextPtr& context, const HttpServerAsyncStreamPtr& stream,
                                                http::HandlerBase* handler) {
  auto stream_handler = static_cast<HttpAsyncStreamFuncHandler*>(handler);
  // executes the filters before rpc methods called
  FilterStatus filter_status =
      GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_PRE_RPC_INVOKE, context);
  if (TRPC_UNLIKELY(filter_status == FilterStatus::REJECT)) {
    GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_POST_RPC_INVOKE, context);
    return HandleError(context, Status{http::ResponseStatus::kForbidden, 0, "request reject"}, stream);
  }
  stream_handler->Handle(context, stream).Then([stream, this, context](Future<> ft) {
    GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_POST_RPC_INVOKE, context);
    if (ft.IsFailed()) {
      Exception ex = ft.GetException();
      Status err_status;
      if (ex.is<Timeout>()) {
        TRPC_FMT_ERROR(ex.what());
        err_status.SetFrameworkRetCode(http::ResponseStatus::kRequestTimeout);
        err_status.SetErrorMessage(ex.what());
      } else {
        // other internal errors within the stream are treated as unknown errors
        TRPC_FMT_ERROR(ex.what());
        err_status.SetFrameworkRetCode(http::ResponseStatus::kInternalServerError);
        err_status.SetErrorMessage("internal error");
      }
      return HandleError(context, std::move(err_status), stream);
    }
    GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_PRE_SEND_MSG, context);
    // stops the stream
    stream->Stop();
  });
}

}  // namespace trpc::stream
