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

#include "trpc/client/http/http_service_proxy.h"

#include <utility>

#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "trpc/codec/client_codec_factory.h"
#include "trpc/util/log/logging.h"

namespace trpc::http {

void HttpServiceProxy::InnerUnaryInvoke(const ClientContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp) {
  context->SetRequestData(const_cast<ProtocolPtr*>(&req));
  context->SetReqEncodeType(serialization::kNoopType);
  context->SetReqEncodeDataType(serialization::kHttpType);

  auto filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);

  if (filter_status != FilterStatus::REJECT) {
    Status status = ServiceProxy::UnaryInvoke(context, req, rsp);
    if (status.OK() && !CheckHttpResponse(context, rsp)) {
      std::string error =
          fmt::format("service name:{},check http reply failed,{}", GetServiceName(), context->GetStatus().ToString());
      TRPC_LOG_ERROR(error);
    }
  } else {
    TRPC_FMT_ERROR("service name:{}, filter pre execute failed.", GetServiceName());
  }
  context->SetResponseData(&rsp);

  filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  context->SetRequestData(nullptr);
  context->SetResponseData(nullptr);
}

Future<ProtocolPtr> HttpServiceProxy::AsyncInnerUnaryInvoke(const ClientContextPtr& context, const ProtocolPtr& req) {
  context->SetRequestData(const_cast<ProtocolPtr*>(&req));
  context->SetReqEncodeType(serialization::kNoopType);
  context->SetReqEncodeDataType(serialization::kHttpType);

  auto filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_status == FilterStatus::REJECT) {
    TRPC_FMT_ERROR("service name:{}, filter execute failed.", GetServiceName());
    const Status& result = context->GetStatus();
    auto fut = MakeExceptionFuture<ProtocolPtr>(CommonException(result.ErrorMessage().c_str()));
    filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
    context->SetRequestData(nullptr);
    return fut;
  }

  return ServiceProxy::AsyncUnaryInvoke(context, req).Then([this, context](Future<ProtocolPtr>&& rsp) {
    if (!rsp.IsFailed()) {
      ProtocolPtr p = rsp.GetValue0();
      // When the call is successful, set the response data (for use by the `CLIENT_POST_RPC_INVOKE` filter).
      context->SetResponseData(&p);
      if (!CheckHttpResponse(context, p)) {
        std::string error = fmt::format("service name:{},check http reply failed,{}", GetServiceName(),
                                        context->GetStatus().ToString());
        TRPC_LOG_ERROR(error);
        filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
        return MakeExceptionFuture<ProtocolPtr>(
            CommonException(context->GetStatus().ErrorMessage().c_str(), context->GetStatus().GetFuncRetCode()));
      }
      filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
      return MakeReadyFuture<ProtocolPtr>(std::move(p));
    }

    filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
    return MakeExceptionFuture<ProtocolPtr>(rsp.GetException());
  });
}

Status HttpServiceProxy::Get(const ClientContextPtr& context, const std::string& url, rapidjson::Document* json) {
  return HttpUnaryInvokeJson(context, OperationType::GET, url, json);
}

Status HttpServiceProxy::GetString(const ClientContextPtr& context, const std::string& url, std::string* rsp_str) {
  return HttpUnaryInvokeString(context, OperationType::GET, url, rsp_str);
}

Status HttpServiceProxy::Get2(const ClientContextPtr& context, const std::string& url, HttpResponse* rsp) {
  return HttpUnaryInvoke(context, OperationType::GET, url, rsp);
}

Status HttpServiceProxy::Head(const ClientContextPtr& context, const std::string& url, HttpResponse* rsp) {
  return HttpUnaryInvoke(context, OperationType::HEAD, url, rsp);
}

Status HttpServiceProxy::Options(const ClientContextPtr& context, const std::string& url, HttpResponse* rsp) {
  return HttpUnaryInvoke(context, OperationType::OPTIONS, url, rsp);
}

Status HttpServiceProxy::Post(const ClientContextPtr& context, const std::string& url, const rapidjson::Document& data,
                              rapidjson::Document* json) {
  return HttpUnaryInvokeJson(context, OperationType::POST, url, data, json);
}

Status HttpServiceProxy::Post(const ClientContextPtr& context, const std::string& url, const std::string& data,
                              std::string* rsp_str) {
  return HttpUnaryInvokeString(context, OperationType::POST, url, data, rsp_str);
}

Status HttpServiceProxy::Post(const ClientContextPtr& context, const std::string& url, std::string&& data,
                              std::string* rsp_str) {
  return HttpUnaryInvokeString(context, OperationType::POST, url, std::move(data), rsp_str);
}

Status HttpServiceProxy::Post(const ClientContextPtr& context, const std::string& url, NoncontiguousBuffer&& data,
                              NoncontiguousBuffer* body) {
  return HttpUnaryInvoke(context, OperationType::POST, url, std::move(data), body);
}

Status HttpServiceProxy::Post2(const ClientContextPtr& context, const std::string& url, const std::string& data,
                               HttpResponse* rsp) {
  return HttpUnaryInvoke(context, OperationType::POST, url, data, rsp);
}

Status HttpServiceProxy::Post2(const ClientContextPtr& context, const std::string& url, std::string&& data,
                               HttpResponse* rsp) {
  return HttpUnaryInvoke(context, POST, url, std::move(data), rsp);
}

Status HttpServiceProxy::Put(const ClientContextPtr& context, const std::string& url, const rapidjson::Document& data,
                             rapidjson::Document* json) {
  return HttpUnaryInvokeJson(context, OperationType::PUT, url, data, json);
}

Status HttpServiceProxy::Put(const ClientContextPtr& context, const std::string& url, const std::string& data,
                             std::string* rsp_str) {
  return HttpUnaryInvokeString(context, OperationType::PUT, url, data, rsp_str);
}

Status HttpServiceProxy::Put(const ClientContextPtr& context, const std::string& url, std::string&& data,
                             std::string* rsp_str) {
  return HttpUnaryInvokeString(context, OperationType::PUT, url, std::move(data), rsp_str);
}

Status HttpServiceProxy::Put(const ClientContextPtr& context, const std::string& url, NoncontiguousBuffer&& data,
                             NoncontiguousBuffer* body) {
  return HttpUnaryInvoke(context, OperationType::PUT, url, std::move(data), body);
}

Status HttpServiceProxy::Put2(const ClientContextPtr& context, const std::string& url, const std::string& data,
                              HttpResponse* rsp) {
  return HttpUnaryInvoke(context, OperationType::PUT, url, data, rsp);
}

Status HttpServiceProxy::Put2(const ClientContextPtr& context, const std::string& url, std::string&& data,
                              HttpResponse* rsp) {
  return HttpUnaryInvoke(context, OperationType::PUT, url, std::move(data), rsp);
}

Status HttpServiceProxy::Patch(const ClientContextPtr& context, const std::string& url, const std::string& data,
                               HttpResponse* rsp) {
  return HttpUnaryInvoke(context, OperationType::PATCH, url, data, rsp);
}

Status HttpServiceProxy::Patch(const ClientContextPtr& context, const std::string& url, std::string&& data,
                               HttpResponse* rsp) {
  return HttpUnaryInvoke(context, OperationType::PATCH, url, std::move(data), rsp);
}

Status HttpServiceProxy::Delete(const ClientContextPtr& context, const std::string& url, const std::string& data,
                                HttpResponse* rsp) {
  return HttpUnaryInvoke(context, OperationType::DELETE, url, data, rsp);
}

Status HttpServiceProxy::Delete(const ClientContextPtr& context, const std::string& url, std::string&& data,
                                HttpResponse* rsp) {
  return HttpUnaryInvoke(context, OperationType::DELETE, url, std::move(data), rsp);
}

stream::HttpClientStreamReaderWriter HttpServiceProxy::Get(const ClientContextPtr& context, const std::string& url) {
  return CreateHttpStreamReaderWriter(context, url, OperationType::GET);
}

stream::HttpClientStreamReaderWriter HttpServiceProxy::Post(const ClientContextPtr& context, const std::string& url) {
  return CreateHttpStreamReaderWriter(context, url, OperationType::POST);
}

stream::HttpClientStreamReaderWriter HttpServiceProxy::Put(const ClientContextPtr& context, const std::string& url) {
  return CreateHttpStreamReaderWriter(context, url, OperationType::PUT);
}

Future<rapidjson::Document> HttpServiceProxy::AsyncGet(const ClientContextPtr& context, const std::string& url) {
  return AsyncHttpUnaryInvokeJson(context, OperationType::GET, url);
}

Future<std::string> HttpServiceProxy::AsyncGetString(const ClientContextPtr& context, const std::string& url) {
  return AsyncHttpUnaryInvokeString(context, OperationType::GET, url);
}

Future<HttpResponse> HttpServiceProxy::AsyncGet2(const ClientContextPtr& context, const std::string& url) {
  return AsyncHttpUnaryInvoke(context, OperationType::GET, url);
}

Future<HttpResponse> HttpServiceProxy::AsyncHead(const ClientContextPtr& context, const std::string& url) {
  return AsyncHttpUnaryInvoke(context, OperationType::HEAD, url);
}

Future<HttpResponse> HttpServiceProxy::AsyncOptions(const ClientContextPtr& context, const std::string& url) {
  return AsyncHttpUnaryInvoke(context, OperationType::OPTIONS, url);
}

Future<rapidjson::Document> HttpServiceProxy::AsyncPost(const ClientContextPtr& context, const std::string& url,
                                                        const rapidjson::Document& data) {
  return AsyncHttpUnaryInvokeJson(context, OperationType::POST, url, data);
}

Future<std::string> HttpServiceProxy::AsyncPost(const ClientContextPtr& context, const std::string& url,
                                                const std::string& data) {
  return AsyncHttpUnaryInvokeString(context, OperationType::POST, url, data);
}

Future<std::string> HttpServiceProxy::AsyncPost(const ClientContextPtr& context, const std::string& url,
                                                std::string&& data) {
  return AsyncHttpUnaryInvokeString(context, OperationType::POST, url, std::move(data));
}

Future<NoncontiguousBuffer> HttpServiceProxy::AsyncPost(const ClientContextPtr& context, const std::string& url,
                                                        NoncontiguousBuffer&& data) {
  return AsyncHttpUnaryInvoke(context, OperationType::POST, url, std::move(data));
}

Future<HttpResponse> HttpServiceProxy::AsyncPost2(const ClientContextPtr& context, const std::string& url,
                                                  const std::string& data) {
  return AsyncHttpUnaryInvoke(context, OperationType::POST, url, data);
}

Future<HttpResponse> HttpServiceProxy::AsyncPost2(const ClientContextPtr& context, const std::string& url,
                                                  std::string&& data) {
  return AsyncHttpUnaryInvoke(context, OperationType::POST, url, std::move(data));
}

Future<rapidjson::Document> HttpServiceProxy::AsyncPut(const ClientContextPtr& context, const std::string& url,
                                                       const rapidjson::Document& data) {
  return AsyncHttpUnaryInvokeJson(context, OperationType::PUT, url, data);
}

Future<std::string> HttpServiceProxy::AsyncPut(const ClientContextPtr& context, const std::string& url,
                                               const std::string& data) {
  return AsyncHttpUnaryInvokeString(context, OperationType::PUT, url, data);
}

Future<std::string> HttpServiceProxy::AsyncPut(const ClientContextPtr& context, const std::string& url,
                                               std::string&& data) {
  return AsyncHttpUnaryInvokeString(context, OperationType::PUT, url, std::move(data));
}

Future<NoncontiguousBuffer> HttpServiceProxy::AsyncPut(const ClientContextPtr& context, const std::string& url,
                                                       NoncontiguousBuffer&& data) {
  return AsyncHttpUnaryInvoke(context, OperationType::PUT, url, std::move(data));
}

Future<HttpResponse> HttpServiceProxy::AsyncPut2(const ClientContextPtr& context, const std::string& url,
                                                 const std::string& data) {
  return AsyncHttpUnaryInvoke(context, OperationType::PUT, url, data);
}

Future<HttpResponse> HttpServiceProxy::AsyncPut2(const ClientContextPtr& context, const std::string& url,
                                                 std::string&& data) {
  return AsyncHttpUnaryInvoke(context, OperationType::PUT, url, std::move(data));
}

Future<HttpResponse> HttpServiceProxy::AsyncDelete(const ClientContextPtr& context, const std::string& url,
                                                   const std::string& data) {
  return AsyncHttpUnaryInvoke(context, OperationType::DELETE, url, data);
}

Future<HttpResponse> HttpServiceProxy::AsyncDelete(const ClientContextPtr& context, const std::string& url,
                                                   std::string&& data) {
  return AsyncHttpUnaryInvoke(context, OperationType::DELETE, url, std::move(data));
}

Future<HttpResponse> HttpServiceProxy::AsyncPatch(const ClientContextPtr& context, const std::string& url,
                                                  const std::string& data) {
  return AsyncHttpUnaryInvoke(context, OperationType::PATCH, url, data);
}

Future<HttpResponse> HttpServiceProxy::AsyncPatch(const ClientContextPtr& context, const std::string& url,
                                                  std::string&& data) {
  return AsyncHttpUnaryInvoke(context, OperationType::PATCH, url, std::move(data));
}

void HttpServiceProxy::ConstructGetRequest(const ClientContextPtr& context, const std::string& url,
                                           HttpRequestProtocol* req) {
  ConstructHttpRequestHeader(context, OperationType::GET, url, req);
}

void HttpServiceProxy::ConstructHeadRequest(const ClientContextPtr& context, const std::string& url,
                                            HttpRequestProtocol* req) {
  ConstructHttpRequestHeader(context, OperationType::HEAD, url, req);
}

void HttpServiceProxy::ConstructOptionsRequest(const ClientContextPtr& context, const std::string& url,
                                               HttpRequestProtocol* req) {
  ConstructHttpRequestHeader(context, OperationType::OPTIONS, url, req);
}

void HttpServiceProxy::ConstructPostRequest(const ClientContextPtr& context, const std::string& url,
                                            const rapidjson::Document& data, HttpRequestProtocol* req) {
  ConstructHttpRequest(context, OperationType::POST, url, data, req);
}

void HttpServiceProxy::ConstructPostRequest(const ClientContextPtr& context, const std::string& url,
                                            const std::string& data, HttpRequestProtocol* req) {
  ConstructHttpRequest(context, OperationType::POST, url, data, req);
}

void HttpServiceProxy::ConstructPostRequest(const ClientContextPtr& context, const std::string& url, std::string&& data,
                                            HttpRequestProtocol* req) {
  ConstructHttpRequest(context, OperationType::POST, url, std::move(data), req);
}

void HttpServiceProxy::ConstructPostRequest(const ClientContextPtr& context, const std::string& url,
                                            NoncontiguousBuffer&& data, HttpRequestProtocol* req) {
  ConstructHttpRequest(context, OperationType::POST, url, std::move(data), req);
}

void HttpServiceProxy::ConstructPutRequest(const ClientContextPtr& context, const std::string& url,
                                           const rapidjson::Document& data, HttpRequestProtocol* req) {
  ConstructHttpRequest(context, OperationType::PUT, url, data, req);
}

void HttpServiceProxy::ConstructPutRequest(const ClientContextPtr& context, const std::string& url,
                                           const std::string& data, HttpRequestProtocol* req) {
  ConstructHttpRequest(context, OperationType::PUT, url, data, req);
}

void HttpServiceProxy::ConstructPutRequest(const ClientContextPtr& context, const std::string& url, std::string&& data,
                                           HttpRequestProtocol* req) {
  ConstructHttpRequest(context, OperationType::PUT, url, std::move(data), req);
}

void HttpServiceProxy::ConstructPutRequest(const ClientContextPtr& context, const std::string& url,
                                           NoncontiguousBuffer&& data, HttpRequestProtocol* req) {
  ConstructHttpRequest(context, OperationType::PUT, url, std::move(data), req);
}

void HttpServiceProxy::ConstructDeleteRequest(const ClientContextPtr& context, const std::string& url,
                                              const rapidjson::Document& data, HttpRequestProtocol* req) {
  ConstructHttpRequest(context, OperationType::DELETE, url, data, req);
}

void HttpServiceProxy::ConstructDeleteRequest(const ClientContextPtr& context, const std::string& url,
                                              const std::string& data, HttpRequestProtocol* req) {
  ConstructHttpRequest(context, OperationType::DELETE, url, data, req);
}

void HttpServiceProxy::ConstructDeleteRequest(const ClientContextPtr& context, const std::string& url,
                                              std::string&& data, HttpRequestProtocol* req) {
  ConstructHttpRequest(context, OperationType::DELETE, url, std::move(data), req);
}

void HttpServiceProxy::ConstructDeleteRequest(const ClientContextPtr& context, const std::string& url,
                                              NoncontiguousBuffer&& data, HttpRequestProtocol* req) {
  ConstructHttpRequest(context, OperationType::DELETE, url, std::move(data), req);
}

void HttpServiceProxy::ConstructPatchRequest(const ClientContextPtr& context, const std::string& url,
                                             const rapidjson::Document& data, HttpRequestProtocol* req) {
  ConstructHttpRequest(context, OperationType::PATCH, url, data, req);
}

void HttpServiceProxy::ConstructPatchRequest(const ClientContextPtr& context, const std::string& url,
                                             const std::string& data, HttpRequestProtocol* req) {
  ConstructHttpRequest(context, OperationType::PATCH, url, data, req);
}

void HttpServiceProxy::ConstructPatchRequest(const ClientContextPtr& context, const std::string& url,
                                             std::string&& data, HttpRequestProtocol* req) {
  ConstructHttpRequest(context, OperationType::PATCH, url, std::move(data), req);
}

void HttpServiceProxy::ConstructPatchRequest(const ClientContextPtr& context, const std::string& url,
                                             NoncontiguousBuffer&& data, HttpRequestProtocol* req) {
  ConstructHttpRequest(context, OperationType::PATCH, url, std::move(data), req);
}

void HttpServiceProxy::ConstructHttpRequest(const ClientContextPtr& context, OperationType http_method,
                                            const std::string& url, HttpRequestProtocol* req) {
  ConstructHttpRequestHeader(context, http_method, url, req);
}

void HttpServiceProxy::ConstructHttpRequest(const ClientContextPtr& context, OperationType http_method,
                                            const std::string& url, const rapidjson::Document& data,
                                            HttpRequestProtocol* req) {
  // parser json data
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  data.Accept(writer);

  context->SetReqEncodeType(serialization::kJsonType);
  ConstructHttpRequestHeader(context, http_method, url, req);
  req->request->SetHeaderIfNotPresent("Content-Length", std::to_string(buffer.GetSize()));
  req->request->SetContent(buffer.GetString());
}

void HttpServiceProxy::ConstructHttpRequest(const ClientContextPtr& context, OperationType http_method,
                                            const std::string& url, const std::string& data, HttpRequestProtocol* req) {
  context->SetReqEncodeType(serialization::kNoopType);
  ConstructHttpRequestHeader(context, http_method, url, req);
  req->request->SetHeaderIfNotPresent("Content-Length", std::to_string(data.size()));
  req->request->SetContent(data);
}

void HttpServiceProxy::ConstructHttpRequest(const ClientContextPtr& context, OperationType http_method,
                                            const std::string& url, std::string&& data, HttpRequestProtocol* req) {
  context->SetReqEncodeType(serialization::kNoopType);
  ConstructHttpRequestHeader(context, http_method, url, req);
  req->request->SetHeaderIfNotPresent("Content-Length", std::to_string(data.size()));
  req->request->SetContent(std::move(data));
}

void HttpServiceProxy::ConstructHttpRequest(const ClientContextPtr& context, OperationType http_method,
                                            const std::string& url, NoncontiguousBuffer&& data,
                                            HttpRequestProtocol* req) {
  context->SetReqEncodeType(serialization::kNoopType);
  ConstructHttpRequestHeader(context, http_method, url, req);
  req->request->SetHeaderIfNotPresent("Content-Length", std::to_string(data.size()));
  req->SetNonContiguousProtocolBody(std::move(data));
}

void HttpServiceProxy::ConstructPBRequest(const ClientContextPtr& context, const std::string& url,
                                          std::string&& content, HttpRequestProtocol* req) {
  context->SetReqEncodeType(serialization::kPbType);
  ConstructHttpRequestHeader(context, OperationType::POST, url, req);
  req->request->SetHeaderIfNotPresent("Content-Length", std::to_string(content.length()));
  req->request->SetContent(std::move(content));
}

bool HttpServiceProxy::CheckHttpResponse(const ClientContextPtr& context, const ProtocolPtr& http_rsp) {
  Status status;
  auto* http_rsp_protocol = static_cast<HttpResponseProtocol*>(http_rsp.get());
  auto http_rsp_status_code = http_rsp_protocol->response.GetStatus();
  // If the response code is not in the 2XX range, it is considered an error response code.
  if (http_rsp_status_code > trpc::http::HttpResponse::StatusCode::kNoContent) {
    std::string error{fmt::format("bad http rsp status:{}", http_rsp_status_code)};
    TRPC_LOG_ERROR(error);

    status.SetFuncRetCode(static_cast<int>(http_rsp_status_code));
    status.SetErrorMessage(std::move(error));
    context->SetStatus(std::move(status));

    return false;
  }
  return true;
}

Status HttpServiceProxy::HttpUnaryInvokeString(const ClientContextPtr& context, OperationType http_method,
                                               const std::string& url, std::string* rsp) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req)) {
    return Status(static_cast<int>(codec::ClientRetCode::ENCODE_ERROR), "unmatched codec");
  }
  FillClientContext(context);

  context->SetReqEncodeType(serialization::kNoopType);
  ConstructHttpRequest(context, http_method, url, req);

  ProtocolPtr& rsp_protocol = context->GetResponse();
  InnerUnaryInvoke(context, req_protocol, rsp_protocol);
  if (context->GetStatus().OK()) {
    auto ret_rsp = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
    *rsp = FlattenSlow(ret_rsp->GetNonContiguousProtocolBody());
  }
  return context->GetStatus();
}

Status HttpServiceProxy::HttpUnaryInvokeString(const ClientContextPtr& context, OperationType http_method,
                                               const std::string& url, std::string&& data, std::string* rsp) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req)) {
    return Status(static_cast<int>(codec::ClientRetCode::ENCODE_ERROR), "unmatched codec");
  }
  FillClientContext(context);

  ConstructHttpRequest(context, http_method, url, std::move(data), req);

  ProtocolPtr& rsp_protocol = context->GetResponse();
  InnerUnaryInvoke(context, req_protocol, rsp_protocol);
  if (context->GetStatus().OK()) {
    auto ret_rsp = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
    *rsp = FlattenSlow(ret_rsp->GetNonContiguousProtocolBody());
  }
  return context->GetStatus();
}

Status HttpServiceProxy::HttpUnaryInvokeString(const ClientContextPtr& context, OperationType http_method,
                                               const std::string& url, const std::string& data, std::string* rsp) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req)) {
    return Status(static_cast<int>(codec::ClientRetCode::ENCODE_ERROR), "unmatched codec");
  }
  FillClientContext(context);

  ConstructHttpRequest(context, http_method, url, data, req);

  ProtocolPtr& rsp_protocol = context->GetResponse();
  InnerUnaryInvoke(context, req_protocol, rsp_protocol);
  if (context->GetStatus().OK()) {
    auto ret_rsp = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
    *rsp = FlattenSlow(ret_rsp->GetNonContiguousProtocolBody());
  }
  return context->GetStatus();
}

Status HttpServiceProxy::HttpUnaryInvokeJson(const ClientContextPtr& context, OperationType http_method,
                                             const std::string& url, rapidjson::Document* rsp) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req)) {
    return Status(static_cast<int>(codec::ClientRetCode::ENCODE_ERROR), "unmatched codec");
  }
  FillClientContext(context);

  context->SetReqEncodeType(serialization::kJsonType);
  ConstructHttpRequest(context, http_method, url, req);

  ProtocolPtr& rsp_protocol = context->GetResponse();
  InnerUnaryInvoke(context, req_protocol, rsp_protocol);
  if (context->GetStatus().OK()) {
    auto ret_rsp = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
    auto buffer = FlattenSlow(ret_rsp->GetNonContiguousProtocolBody());
    rsp->Parse(buffer.c_str(), buffer.size());
    // In accordance with business requirements, if it is empty, `status.OK()` is considered true.
    if (!buffer.empty() && rsp->HasParseError()) {
      Status status;
      std::string error{fmt::format("Http JsonParse Failed: {}", rapidjson::GetParseError_En(rsp->GetParseError()))};
      TRPC_LOG_ERROR(error);
      status.SetFrameworkRetCode(codec::GetDefaultClientRetCode(codec::ClientRetCode::DECODE_ERROR));
      status.SetErrorMessage(std::move(error));
      context->SetStatus(std::move(status));
    }
  }
  return context->GetStatus();
}

Status HttpServiceProxy::HttpUnaryInvokeJson(const ClientContextPtr& context, OperationType http_method,
                                             const std::string& url, const rapidjson::Document& data,
                                             rapidjson::Document* rsp) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req)) {
    return Status(static_cast<int>(codec::ClientRetCode::ENCODE_ERROR), "unmatched codec");
  }
  FillClientContext(context);

  ConstructHttpRequest(context, http_method, url, data, req);

  ProtocolPtr& rsp_protocol = context->GetResponse();
  InnerUnaryInvoke(context, req_protocol, rsp_protocol);
  if (context->GetStatus().OK()) {
    auto ret_rsp = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
    auto buffer = FlattenSlow(ret_rsp->GetNonContiguousProtocolBody());
    rsp->Parse(buffer.c_str(), buffer.size());
    // In accordance with business requirements, if it is empty, `status.OK()` is considered true.
    if (!buffer.empty() && rsp->HasParseError()) {
      Status status;
      std::string error{fmt::format("Http JsonParse Failed: {}", rapidjson::GetParseError_En(rsp->GetParseError()))};
      TRPC_LOG_ERROR(error);
      status.SetFrameworkRetCode(codec::GetDefaultClientRetCode(codec::ClientRetCode::DECODE_ERROR));
      status.SetErrorMessage(std::move(error));
      context->SetStatus(std::move(status));
    }
  }
  return context->GetStatus();
}

Status HttpServiceProxy::HttpUnaryInvoke(const ClientContextPtr& context, OperationType http_method,
                                         const std::string& url, NoncontiguousBuffer&& data, NoncontiguousBuffer* rsp) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req)) {
    return Status(static_cast<int>(codec::ClientRetCode::ENCODE_ERROR), "unmatched codec");
  }
  FillClientContext(context);

  ConstructHttpRequest(context, http_method, url, std::move(data), req);

  ProtocolPtr& rsp_protocol = context->GetResponse();
  InnerUnaryInvoke(context, req_protocol, rsp_protocol);
  if (context->GetStatus().OK()) {
    auto ret_rsp = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
    *rsp = ret_rsp->GetNonContiguousProtocolBody();
  }
  return context->GetStatus();
}

Status HttpServiceProxy::HttpUnaryInvoke(const ClientContextPtr& context, OperationType http_method,
                                         const std::string& url, HttpResponse* rsp) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req)) {
    return Status(static_cast<int>(codec::ClientRetCode::ENCODE_ERROR), "unmatched codec");
  }
  FillClientContext(context);

  context->SetReqEncodeType(serialization::kNoopType);
  ConstructHttpRequest(context, http_method, url, req);

  ProtocolPtr& rsp_protocol = context->GetResponse();
  InnerUnaryInvoke(context, req_protocol, rsp_protocol);
  if (context->GetStatus().OK()) {
    auto ret_rsp = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
    *rsp = std::move(ret_rsp->response);
  }
  return context->GetStatus();
}

Status HttpServiceProxy::HttpUnaryInvoke(const ClientContextPtr& context, OperationType http_method,
                                         const std::string& url, const std::string& data, HttpResponse* rsp) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req)) {
    return Status(static_cast<int>(codec::ClientRetCode::ENCODE_ERROR), "unmatched codec");
  }
  FillClientContext(context);

  ConstructHttpRequest(context, http_method, url, data, req);

  ProtocolPtr& rsp_protocol = context->GetResponse();
  InnerUnaryInvoke(context, req_protocol, rsp_protocol);
  if (context->GetStatus().OK()) {
    auto ret_rsp = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
    *rsp = std::move(ret_rsp->response);
  }
  return context->GetStatus();
}

Status HttpServiceProxy::HttpUnaryInvoke(const ClientContextPtr& context, OperationType http_method,
                                         const std::string& url, std::string&& data, HttpResponse* rsp) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req)) {
    return Status(static_cast<int>(codec::ClientRetCode::ENCODE_ERROR), "unmatched codec");
  }
  FillClientContext(context);

  ConstructHttpRequest(context, http_method, url, std::move(data), req);

  ProtocolPtr& rsp_protocol = context->GetResponse();
  InnerUnaryInvoke(context, req_protocol, rsp_protocol);
  if (context->GetStatus().OK()) {
    auto ret_rsp = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
    *rsp = std::move(ret_rsp->response);
  }
  return context->GetStatus();
}

Future<std::string> HttpServiceProxy::AsyncHttpUnaryInvokeString(const ClientContextPtr& context,
                                                                 OperationType http_method, const std::string& url) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req)) {
    return MakeExceptionFuture<std::string>(CommonException("unmatched codec"));
  }
  FillClientContext(context);

  ConstructHttpRequest(context, http_method, url, req);

  return AsyncInnerUnaryInvoke(context, req_protocol).Then([context](Future<ProtocolPtr>&& rsp_fut) {
    if (!rsp_fut.IsFailed()) {
      ProtocolPtr p = rsp_fut.GetValue0();
      auto* rsp = static_cast<HttpResponseProtocol*>(p.get());
      return MakeReadyFuture<std::string>(FlattenSlow(rsp->GetNonContiguousProtocolBody()));
    }
    return MakeExceptionFuture<std::string>(rsp_fut.GetException());
  });
}

Future<std::string> HttpServiceProxy::AsyncHttpUnaryInvokeString(const ClientContextPtr& context,
                                                                 OperationType http_method, const std::string& url,
                                                                 std::string&& data) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req)) {
    return MakeExceptionFuture<std::string>(CommonException("unmatched codec"));
  }
  FillClientContext(context);

  ConstructHttpRequest(context, http_method, url, std::move(data), req);

  return AsyncInnerUnaryInvoke(context, req_protocol).Then([context](Future<ProtocolPtr>&& rsp_fut) {
    if (!rsp_fut.IsFailed()) {
      ProtocolPtr p = rsp_fut.GetValue0();
      auto* rsp = static_cast<HttpResponseProtocol*>(p.get());
      return MakeReadyFuture<std::string>(FlattenSlow(rsp->GetNonContiguousProtocolBody()));
    }
    return MakeExceptionFuture<std::string>(rsp_fut.GetException());
  });
}

Future<std::string> HttpServiceProxy::AsyncHttpUnaryInvokeString(const ClientContextPtr& context,
                                                                 OperationType http_method, const std::string& url,
                                                                 const std::string& data) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req)) {
    return MakeExceptionFuture<std::string>(CommonException("unmatched codec"));
  }
  FillClientContext(context);

  ConstructHttpRequest(context, http_method, url, data, req);

  return AsyncInnerUnaryInvoke(context, req_protocol).Then([context](Future<ProtocolPtr>&& rsp_fut) {
    if (!rsp_fut.IsFailed()) {
      ProtocolPtr p = rsp_fut.GetValue0();
      auto* rsp = static_cast<HttpResponseProtocol*>(p.get());
      return MakeReadyFuture<std::string>(FlattenSlow(rsp->GetNonContiguousProtocolBody()));
    }
    return MakeExceptionFuture<std::string>(rsp_fut.GetException());
  });
}

Future<rapidjson::Document> HttpServiceProxy::AsyncHttpUnaryInvokeJson(const ClientContextPtr& context,
                                                                       OperationType http_method,
                                                                       const std::string& url,
                                                                       const rapidjson::Document& data) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req)) {
    return MakeExceptionFuture<rapidjson::Document>(CommonException("unmatched codec"));
  }
  FillClientContext(context);

  ConstructHttpRequest(context, http_method, url, data, req);

  return AsyncInnerUnaryInvoke(context, req_protocol).Then([context](Future<ProtocolPtr>&& rsp_fut) {
    if (!rsp_fut.IsFailed()) {
      ProtocolPtr p = rsp_fut.GetValue0();
      auto* rsp = static_cast<HttpResponseProtocol*>(p.get());
      auto rsp_str = FlattenSlow(rsp->GetNonContiguousProtocolBody());
      rapidjson::Document rsp_json;
      rsp_json.Parse(rsp_str.c_str(), rsp_str.size());
      if (!rsp_str.empty() && rsp_json.HasParseError()) {
        std::string error{
            fmt::format("Http JsonParse Failed: {}", rapidjson::GetParseError_En(rsp_json.GetParseError()))};
        TRPC_LOG_ERROR(error);
        return MakeExceptionFuture<rapidjson::Document>(
            CommonException(error.c_str(), codec::GetDefaultClientRetCode(codec::ClientRetCode::DECODE_ERROR)));
      }
      return MakeReadyFuture<rapidjson::Document>(std::move(rsp_json));
    }
    return MakeExceptionFuture<rapidjson::Document>(rsp_fut.GetException());
  });
}

Future<rapidjson::Document> HttpServiceProxy::AsyncHttpUnaryInvokeJson(const ClientContextPtr& context,
                                                                       OperationType http_method,
                                                                       const std::string& url) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req)) {
    return MakeExceptionFuture<rapidjson::Document>(CommonException("unmatched codec"));
  }
  FillClientContext(context);

  context->SetReqEncodeType(serialization::kJsonType);
  ConstructHttpRequest(context, http_method, url, req);

  return AsyncInnerUnaryInvoke(context, req_protocol).Then([context](Future<ProtocolPtr>&& rsp_fut) {
    if (!rsp_fut.IsFailed()) {
      ProtocolPtr p = rsp_fut.GetValue0();
      auto* rsp = static_cast<HttpResponseProtocol*>(p.get());
      auto rsp_str = FlattenSlow(rsp->GetNonContiguousProtocolBody());
      rapidjson::Document rsp_json;
      rsp_json.Parse(rsp_str.c_str(), rsp_str.size());
      if (!rsp_str.empty() && rsp_json.HasParseError()) {
        std::string error{
            fmt::format("Http JsonParse Failed: {}", rapidjson::GetParseError_En(rsp_json.GetParseError()))};
        TRPC_LOG_ERROR(error);
        return MakeExceptionFuture<rapidjson::Document>(
            CommonException(error.c_str(), codec::GetDefaultClientRetCode(codec::ClientRetCode::DECODE_ERROR)));
      }
      return MakeReadyFuture<rapidjson::Document>(std::move(rsp_json));
    }
    return MakeExceptionFuture<rapidjson::Document>(rsp_fut.GetException());
  });
}

Future<NoncontiguousBuffer> HttpServiceProxy::AsyncHttpUnaryInvoke(const ClientContextPtr& context,
                                                                   OperationType http_method, const std::string& url,
                                                                   NoncontiguousBuffer&& data) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req)) {
    return MakeExceptionFuture<NoncontiguousBuffer>(CommonException("unmatched codec"));
  }
  FillClientContext(context);

  ConstructHttpRequest(context, http_method, url, std::move(data), req);

  return AsyncInnerUnaryInvoke(context, req_protocol).Then([context](Future<ProtocolPtr>&& rsp_fut) {
    if (!rsp_fut.IsFailed()) {
      ProtocolPtr p = rsp_fut.GetValue0();
      auto* rsp = static_cast<HttpResponseProtocol*>(p.get());
      return MakeReadyFuture<NoncontiguousBuffer>(rsp->GetNonContiguousProtocolBody());
    }
    return MakeExceptionFuture<NoncontiguousBuffer>(rsp_fut.GetException());
  });
}

Future<HttpResponse> HttpServiceProxy::AsyncHttpUnaryInvoke(const ClientContextPtr& context, OperationType http_method,
                                                            const std::string& url) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req)) {
    return MakeExceptionFuture<HttpResponse>(CommonException("unmatched codec"));
  }
  FillClientContext(context);

  ConstructHttpRequest(context, http_method, url, req);

  return AsyncInnerUnaryInvoke(context, req_protocol).Then([context](Future<ProtocolPtr>&& rsp_fut) {
    if (!rsp_fut.IsFailed()) {
      ProtocolPtr p = rsp_fut.GetValue0();
      auto* rsp = static_cast<HttpResponseProtocol*>(p.get());
      return MakeReadyFuture<HttpResponse>(std::move(rsp->response));
    }
    return MakeExceptionFuture<HttpResponse>(rsp_fut.GetException());
  });
}

Future<HttpResponse> HttpServiceProxy::AsyncHttpUnaryInvoke(const ClientContextPtr& context, OperationType http_method,
                                                            const std::string& url, const std::string& data) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  if (TRPC_UNLIKELY(!req)) {
    return MakeExceptionFuture<HttpResponse>(CommonException("unmatched codec"));
  }
  FillClientContext(context);

  ConstructHttpRequest(context, http_method, url, data, req);

  return AsyncInnerUnaryInvoke(context, req_protocol).Then([context](Future<ProtocolPtr>&& rsp_fut) {
    if (!rsp_fut.IsFailed()) {
      ProtocolPtr p = rsp_fut.GetValue0();
      auto* rsp = static_cast<HttpResponseProtocol*>(p.get());
      return MakeReadyFuture<HttpResponse>(std::move(rsp->response));
    }
    return MakeExceptionFuture<HttpResponse>(rsp_fut.GetException());
  });
}

Future<HttpResponse> HttpServiceProxy::AsyncHttpUnaryInvoke(const ClientContextPtr& context, OperationType http_method,
                                                            const std::string& url, std::string&& data) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req)) {
    return MakeExceptionFuture<HttpResponse>(CommonException("unmatched codec"));
  }
  FillClientContext(context);

  ConstructHttpRequest(context, http_method, url, std::move(data), req);

  return AsyncInnerUnaryInvoke(context, req_protocol).Then([context](Future<ProtocolPtr>&& rsp_fut) {
    if (!rsp_fut.IsFailed()) {
      ProtocolPtr p = rsp_fut.GetValue0();
      auto* rsp = static_cast<HttpResponseProtocol*>(p.get());
      return MakeReadyFuture<HttpResponse>(std::move(rsp->response));
    }
    return MakeExceptionFuture<HttpResponse>(rsp_fut.GetException());
  });
}

void HttpServiceProxy::ConstructHttpRequestHeader(const ClientContextPtr& context, OperationType http_method,
                                                  const std::string& url) {
  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  TRPC_ASSERT(req && "client context request is nullptr or unmatched codec");
  req->SetFromHttpServiceProxy(true);

  ConstructHttpRequestHeader(context, http_method, url, req);
}

void HttpServiceProxy::ConstructRpcHttpRequestHeader(const ClientContextPtr& context) {
  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  TRPC_ASSERT(req && "client context request is nullptr or unmatched codec");
  req->SetFromHttpServiceProxy(true);
  req->request->SetMethodType(POST);
  req->request->SetVersion("1.1");
  req->request->SetUrl(context->GetFuncName());
  for (const auto& kv : context->GetHttpHeaders()) {
    req->request->AddHeader(kv.first, kv.second);
  }
  req->request->SetHeaderIfNotPresent("Content-Type", "application/pb");
}

void HttpServiceProxy::ConstructHttpRequestContext(const ClientContextPtr& context, OperationType http_method,
                                                   uint8_t encode_type, const std::string& url) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  context->SetReqEncodeType(encode_type);
  ConstructHttpRequestHeader(context, http_method, url);
}

void HttpServiceProxy::ConstructHttpRequestHeader(const ClientContextPtr& context, OperationType http_method,
                                                  const std::string& url, HttpRequestProtocol* req) {
  UrlParser url_parse(url);

  req->request->SetMethodType(http_method);
  req->request->SetVersion("1.1");
  req->request->SetUrl(url_parse.Request());
  for (const auto& [key, value] : context->GetHttpHeaders()) {
    req->request->AddHeader(key, value);
  }
  std::string host = url_parse.Hostname();
  if (!url_parse.Port().empty()) {
    host.append(":").append(url_parse.Port());
  }
  req->request->SetHeaderIfNotPresent("Host", host);

  context->SetCurrentContextExt(static_cast<uint32_t>(http_method));

  SetHttpRequestEncodeType(context, http_method, req);
}

void HttpServiceProxy::SetHttpRequestEncodeType(const ClientContextPtr& context, OperationType http_method,
                                                HttpRequestProtocol* req) {
  if (http_method == OperationType::GET || http_method == OperationType::HEAD ||
      http_method == OperationType::OPTIONS) {
    return;
  } else if (http_method == OperationType::POST || http_method == OperationType::PUT ||
             http_method == OperationType::PATCH || http_method == OperationType::DELETE) {
    switch (context->GetReqEncodeType()) {
      case serialization::kPbType:
        req->request->SetHeaderIfNotPresent("Content-Type", "application/pb");
        break;
      case serialization::kJsonType:
        req->request->SetHeaderIfNotPresent("Content-Type", "application/json");
        req->request->SetHeaderIfNotPresent("Accept", "application/json");
        break;
      case serialization::kNoopType:
        // The `Content-Type` should not be set to `json` here (it will be set when using `Post/PostString`). However,
        // since some businesses may use the `Post` interface as `json` by default, an alarm will be triggered in this
        // case.
        if (!req->request->HasHeader("Content-Type")) {
          TRPC_LOG_WARN(
              "Content-Type will be set as json(correct is text/plain) due to history issue, please explict set before "
              "invoke");
          req->request->SetHeader("Content-Type", "application/json");
        }
        req->request->SetHeaderIfNotPresent("Accept", "*/*");
        break;
      default:
        TRPC_LOG_ERROR("unsupported encode_type:" << context->GetReqEncodeType());
        break;
    }
  } else {
    TRPC_LOG_ERROR("unsupported http method:" << http_method);
    return;
  }
}

stream::HttpClientStreamReaderWriter HttpServiceProxy::CreateHttpStreamReaderWriter(const ClientContextPtr& context,
                                                                                    const std::string& url,
                                                                                    OperationType http_method) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto req = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  if (TRPC_UNLIKELY(!req)) {
    return stream::HttpClientStreamReaderWriter(MakeRefCounted<stream::HttpClientStream>(
        Status(static_cast<int>(codec::ClientRetCode::ENCODE_ERROR), "unmatched codec"), true));
  }
  FillClientContext(context);
  ConstructHttpRequestHeader(context, http_method, url, req);

  auto stream_provider = ServiceProxy::SelectStreamProvider(context, nullptr);
  if (!stream_provider->GetStatus().OK()) {
    return stream::HttpClientStreamReaderWriter(
        MakeRefCounted<stream::HttpClientStream>(stream_provider->GetStatus(), true));
  }

  auto http_stream_provider = static_pointer_cast<stream::HttpClientStream>(stream_provider);
  http_stream_provider->SetCapacity(GetServiceProxyOption()->max_packet_size > 0
                                        ? GetServiceProxyOption()->max_packet_size
                                        : std::numeric_limits<size_t>::max());
  http_stream_provider->SetMethod(http_method);
  http_stream_provider->SetHttpRequestProtocol(req);
  http_stream_provider->SendRequestHeader();

  return stream::HttpClientStreamReaderWriter(std::move(http_stream_provider));
}

}  // namespace trpc::http
