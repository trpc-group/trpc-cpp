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

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "rapidjson/document.h"

#include "trpc/client/rpc_service_proxy.h"
#include "trpc/codec/http/http_protocol.h"
#include "trpc/stream/http/http_client_stream.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"

namespace trpc::http {

const rapidjson::Document kEmptyRapidJsonDoc;

/// @brief HTTP client proxy instance, supporting HTTP methods such as GET, HEAD, POST, PUT, DELETE, and OPTIONS.
class HttpServiceProxy : public trpc::RpcServiceProxy {
 public:
  /// @brief Gets a JSON response content from HTTP server.
  Status Get(const ClientContextPtr& context, const std::string& url, rapidjson::Document* json);
  /// @brief Gets a string response content from HTTP server.
  Status GetString(const ClientContextPtr& context, const std::string& url, std::string* rsp_str);
  /// @brief Gets an HTTP response from  HTTP server.
  Status Get2(const ClientContextPtr& context, const std::string& url, HttpResponse* rsp);

  /// @brief Gets an HTTP response of an HTTP HEAD method request from HTTP server.
  Status Head(const ClientContextPtr& context, const std::string& url, HttpResponse* rsp);

  /// @brief Gets an HTTP response of an HTTP OPTIONS method request from HTTP server.
  Status Options(const ClientContextPtr& context, const std::string& url, HttpResponse* rsp);

  /// @brief Posts a JSON as the request body to HTTP server, then gets the JSON content of response.
  Status Post(const ClientContextPtr& context, const std::string& url, const rapidjson::Document& data,
              rapidjson::Document* rsp);

  /// @brief Posts a string as the request body to HTTP server, then gets the string content of response.
  Status Post(const ClientContextPtr& context, const std::string& url, const std::string& data, std::string* rsp_str);
  Status Post(const ClientContextPtr& context, const std::string& url, std::string&& data, std::string* rsp_str);
  Status Post(const ClientContextPtr& context, const std::string& url, NoncontiguousBuffer&& data,
              NoncontiguousBuffer* body);

  /// @brief Posts a string as the request body to HTTP server, then gets an HTTP response.
  Status Post2(const ClientContextPtr& context, const std::string& url, const std::string& data, HttpResponse* rsp);
  Status Post2(const ClientContextPtr& context, const std::string& url, std::string&& data, HttpResponse* rsp);

  /// @brief Puts a JSON as the request body to HTTP server, then gets the JSON content of response.
  Status Put(const ClientContextPtr& context, const std::string& url, const rapidjson::Document& data,
             rapidjson::Document* rsp);
  /// @brief Puts a string as the request body to HTTP server, then gets the string content of response.
  Status Put(const ClientContextPtr& context, const std::string& url, const std::string& data, std::string* rsp_str);
  Status Put(const ClientContextPtr& context, const std::string& url, std::string&& data, std::string* rsp_str);
  Status Put(const ClientContextPtr& context, const std::string& url, NoncontiguousBuffer&& data,
             NoncontiguousBuffer* body);

  /// @brief Puts a string as the request body to HTTP server, then gets the HTTP response.
  Status Put2(const ClientContextPtr& context, const std::string& url, const std::string& data, HttpResponse* rsp);
  Status Put2(const ClientContextPtr& context, const std::string& url, std::string&& data, HttpResponse* rsp);

  /// @brief Sends an HTTP PATCH request with a string as the request body to HTTP server, then gets the HTTP response.
  Status Patch(const ClientContextPtr& context, const std::string& url, const std::string& data, HttpResponse* rsp);
  Status Patch(const ClientContextPtr& context, const std::string& url, std::string&& data, HttpResponse* rsp);

  /// @brief Sends a HTTP DELETE request with a string as the request body to HTTP server, then gets the HTTP response.
  Status Delete(const ClientContextPtr& context, const std::string& url, const std::string& data, HttpResponse* rsp);
  Status Delete(const ClientContextPtr& context, const std::string& url, std::string&& data, HttpResponse* rsp);

  /// @brief Downloads a file from HTTP server(it's synchronous streaming interface).
  stream::HttpClientStreamReaderWriter Get(const ClientContextPtr& context, const std::string& url);

  /// @brief Uploads a file to HTTP server(it's synchronous streaming interface).
  stream::HttpClientStreamReaderWriter Post(const ClientContextPtr& context, const std::string& url);

  /// @brief Uploads a file to HTTP server(it's synchronous streaming interface).
  stream::HttpClientStreamReaderWriter Put(const ClientContextPtr& context, const std::string& url);

  /// @brief HTTP synchronous call.
  ///
  /// @param context Client context
  /// @tparam RequestMessage Request message type.
  /// @tparam ResponseMessage Response message type.
  /// @param req is the request message.
  /// @param rsp is the response message.
  /// @return Status Result of the interface execution; if successful, `Status::OK()` is true, otherwise it indicates
  /// that the call failed.
  template <class RequestMessage, class ResponseMessage>
  Status UnaryInvoke(const ClientContextPtr& context, const RequestMessage& req, ResponseMessage* rsp);

  /// @brief Issues HTTP synchronous call with http::Request and http::Response.
  ///
  /// @param context Client context
  /// @tparam RequestMessage is http::Request .
  /// @tparam ResponseMessage is http::Response.
  /// @param req is the request message.
  /// @param rsp is the response message.
  /// @return Status Result of the interface execution; if successful, `Status::OK()` is true, otherwise it indicates
  /// that the call failed.
  template <class RequestMessage, class ResponseMessage>
  Status HttpUnaryInvoke(const ClientContextPtr& context, const RequestMessage& req, ResponseMessage* rsp);

  /// @brief Issues HTTP synchronous call with a PB message.
  ///
  /// @param context Client context
  /// @tparam RequestMessage Request message type.
  /// @tparam ResponseMessage Response message type.
  /// @param req is the request message.
  /// @param rsp is the response message.
  /// @return Status Result of the interface execution; if successful, `Status::OK()` is true, otherwise it indicates
  /// that the call failed.
  template <class RequestMessage, class ResponseMessage>
  Status PBInvoke(const ClientContextPtr& context, const std::string& url, const RequestMessage& req,
                  ResponseMessage* rsp);

  Future<rapidjson::Document> AsyncGet(const ClientContextPtr& context, const std::string& url);
  Future<std::string> AsyncGetString(const ClientContextPtr& context, const std::string& url);
  Future<HttpResponse> AsyncGet2(const ClientContextPtr& context, const std::string& url);
  Future<HttpResponse> AsyncHead(const ClientContextPtr& context, const std::string& url);
  Future<HttpResponse> AsyncOptions(const ClientContextPtr& context, const std::string& url);
  Future<rapidjson::Document> AsyncPost(const ClientContextPtr& context, const std::string& url,
                                        const rapidjson::Document& data);
  Future<std::string> AsyncPost(const ClientContextPtr& context, const std::string& url, const std::string& data);
  Future<std::string> AsyncPost(const ClientContextPtr& context, const std::string& url, std::string&& data);
  Future<NoncontiguousBuffer> AsyncPost(const ClientContextPtr& context, const std::string& url,
                                        NoncontiguousBuffer&& data);
  Future<HttpResponse> AsyncPost2(const ClientContextPtr& context, const std::string& url, const std::string& data);
  Future<HttpResponse> AsyncPost2(const ClientContextPtr& context, const std::string& url, std::string&& data);
  Future<rapidjson::Document> AsyncPut(const ClientContextPtr& context, const std::string& url,
                                       const rapidjson::Document& data);
  Future<std::string> AsyncPut(const ClientContextPtr& context, const std::string& url, const std::string& data);
  Future<std::string> AsyncPut(const ClientContextPtr& context, const std::string& url, std::string&& data);
  Future<NoncontiguousBuffer> AsyncPut(const ClientContextPtr& context, const std::string& url,
                                       NoncontiguousBuffer&& data);
  Future<HttpResponse> AsyncPut2(const ClientContextPtr& context, const std::string& url, const std::string& data);
  Future<HttpResponse> AsyncPut2(const ClientContextPtr& context, const std::string& url, std::string&& data);

  Future<HttpResponse> AsyncDelete(const ClientContextPtr& context, const std::string& url, const std::string& data);
  Future<HttpResponse> AsyncDelete(const ClientContextPtr& context, const std::string& url, std::string&& data);

  Future<HttpResponse> AsyncPatch(const ClientContextPtr& context, const std::string& url, const std::string& data);
  Future<HttpResponse> AsyncPatch(const ClientContextPtr& context, const std::string& url, std::string&& data);

  template <class RequestMessage, class ResponseMessage>
  Future<ResponseMessage> AsyncUnaryInvoke(const ClientContextPtr& context, const RequestMessage& req);

  template <class RequestMessage, class ResponseMessage>
  Future<ResponseMessage> AsyncHttpUnaryInvoke(const ClientContextPtr& context, const RequestMessage& req);

  template <class RequestMessage, class ResponseMessage>
  Future<ResponseMessage> AsyncPBInvoke(const ClientContextPtr& context, const std::string& url,
                                        const RequestMessage& req);

 protected:
  /// @brief Checks if the received `HttpResponse` is normal (such as an abnormal response code), which will be used by
  /// the business.
  virtual bool CheckHttpResponse(const ClientContextPtr& context, const ProtocolPtr& http_rsp);

 protected:
  /// @private For internal use purpose only.
  void ConstructGetRequest(const ClientContextPtr& context, const std::string& url, HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructHeadRequest(const ClientContextPtr& context, const std::string& url, HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructOptionsRequest(const ClientContextPtr& context, const std::string& url, HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructPostRequest(const ClientContextPtr& context, const std::string& url, const rapidjson::Document& data,
                            HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructPostRequest(const ClientContextPtr& context, const std::string& url, const std::string& data,
                            HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructPostRequest(const ClientContextPtr& context, const std::string& url, std::string&& data,
                            HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructPostRequest(const ClientContextPtr& context, const std::string& url, NoncontiguousBuffer&& data,
                            HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructPutRequest(const ClientContextPtr& context, const std::string& url, const rapidjson::Document& data,
                           HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructPutRequest(const ClientContextPtr& context, const std::string& url, const std::string& data,
                           HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructPutRequest(const ClientContextPtr& context, const std::string& url, std::string&& data,
                           HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructPutRequest(const ClientContextPtr& context, const std::string& url, NoncontiguousBuffer&& data,
                           HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructDeleteRequest(const ClientContextPtr& context, const std::string& url, const rapidjson::Document& data,
                              HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructDeleteRequest(const ClientContextPtr& context, const std::string& url, const std::string& data,
                              HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructDeleteRequest(const ClientContextPtr& context, const std::string& url, std::string&& data,
                              HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructDeleteRequest(const ClientContextPtr& context, const std::string& url, NoncontiguousBuffer&& data,
                              HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructPatchRequest(const ClientContextPtr& context, const std::string& url, const rapidjson::Document& data,
                             HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructPatchRequest(const ClientContextPtr& context, const std::string& url, const std::string& data,
                             HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructPatchRequest(const ClientContextPtr& context, const std::string& url, std::string&& data,
                             HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructPatchRequest(const ClientContextPtr& context, const std::string& url, NoncontiguousBuffer&& data,
                             HttpRequestProtocol* req);
  /// @private For internal use purpose only.
  void ConstructPBRequest(const ClientContextPtr& context, const std::string& url, std::string&& content,
                          HttpRequestProtocol* req);

  /// @brief Creates an HTTP client stream reader and writer.
  /// @private For internal use purpose only.
  stream::HttpClientStreamReaderWriter CreateHttpStreamReaderWriter(const ClientContextPtr& context,
                                                                    const std::string& url, OperationType http_method);

 private:
  Status HttpUnaryInvoke(const ClientContextPtr& context, OperationType http_method, const std::string& url,
                         NoncontiguousBuffer&& data, NoncontiguousBuffer* rsp);

  Status HttpUnaryInvokeString(const ClientContextPtr& context, OperationType http_method, const std::string& url,
                               std::string* rsp);

  Status HttpUnaryInvokeString(const ClientContextPtr& context, OperationType http_method, const std::string& url,
                               std::string&& data, std::string* rsp);

  Status HttpUnaryInvokeString(const ClientContextPtr& context, OperationType http_method, const std::string& url,
                               const std::string& data, std::string* rsp);

  Status HttpUnaryInvokeJson(const ClientContextPtr& context, OperationType http_method, const std::string& url,
                             rapidjson::Document* rsp);

  Status HttpUnaryInvokeJson(const ClientContextPtr& context, OperationType http_method, const std::string& url,
                             const rapidjson::Document& data, rapidjson::Document* rsp);

  Status HttpUnaryInvoke(const ClientContextPtr& context, OperationType http_method, const std::string& url,
                         HttpResponse* rsp);

  Status HttpUnaryInvoke(const ClientContextPtr& context, OperationType http_method, const std::string& url,
                         const std::string& data, HttpResponse* rsp);

  Status HttpUnaryInvoke(const ClientContextPtr& context, OperationType http_method, const std::string& url,
                         std::string&& data, HttpResponse* rsp);

  Future<std::string> AsyncHttpUnaryInvokeString(const ClientContextPtr& context, OperationType http_method,
                                                 const std::string& url);

  Future<std::string> AsyncHttpUnaryInvokeString(const ClientContextPtr& context, OperationType http_method,
                                                 const std::string& url, std::string&& data);

  Future<std::string> AsyncHttpUnaryInvokeString(const ClientContextPtr& context, OperationType http_method,
                                                 const std::string& url, const std::string& data);

  Future<rapidjson::Document> AsyncHttpUnaryInvokeJson(const ClientContextPtr& context, OperationType http_method,
                                                       const std::string& url, const rapidjson::Document& data);

  Future<rapidjson::Document> AsyncHttpUnaryInvokeJson(const ClientContextPtr& context, OperationType http_method,
                                                       const std::string& url);

  Future<NoncontiguousBuffer> AsyncHttpUnaryInvoke(const ClientContextPtr& context, OperationType http_method,
                                                   const std::string& url, NoncontiguousBuffer&& data);

  Future<HttpResponse> AsyncHttpUnaryInvoke(const ClientContextPtr& context, OperationType http_method,
                                            const std::string& url);

  Future<HttpResponse> AsyncHttpUnaryInvoke(const ClientContextPtr& context, OperationType http_method,
                                            const std::string& url, const std::string& data);

  Future<HttpResponse> AsyncHttpUnaryInvoke(const ClientContextPtr& context, OperationType http_method,
                                            const std::string& url, std::string&& data);

  void InnerUnaryInvoke(const ClientContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp);

  Future<ProtocolPtr> AsyncInnerUnaryInvoke(const ClientContextPtr& context, const ProtocolPtr& req);

  void ConstructHttpRequest(const ClientContextPtr& context, OperationType http_method, const std::string& url,
                            HttpRequestProtocol* req);

  void ConstructHttpRequest(const ClientContextPtr& context, OperationType http_method, const std::string& url,
                            const rapidjson::Document& data, HttpRequestProtocol* req);
  void ConstructHttpRequest(const ClientContextPtr& context, OperationType http_method, const std::string& url,
                            const std::string& data, HttpRequestProtocol* req);
  void ConstructHttpRequest(const ClientContextPtr& context, OperationType http_method, const std::string& url,
                            std::string&& data, HttpRequestProtocol* req);
  void ConstructHttpRequest(const ClientContextPtr& context, OperationType http_method, const std::string& url,
                            NoncontiguousBuffer&& data, HttpRequestProtocol* req);

  void ConstructHttpRequestContext(const ClientContextPtr& context, OperationType http_method, uint8_t encode_type,
                                   const std::string& url);
  // @brief Constructs a standard HTTP request header (including HTTP request line and HTTP request headers).
  void ConstructHttpRequestHeader(const ClientContextPtr& context, OperationType http_method, const std::string& url);
  void ConstructHttpRequestHeader(const ClientContextPtr& context, OperationType http_method, const std::string& url,
                                  HttpRequestProtocol* req);

  // @brief Constructs an RPC HTTP request header (including HTTP request line and HTTP request headers).
  void ConstructRpcHttpRequestHeader(const ClientContextPtr& context);

  void SetHttpRequestEncodeType(const ClientContextPtr& context, OperationType http_method, HttpRequestProtocol* req);
};

template <class RequestMessage, class ResponseMessage>
Status HttpServiceProxy::PBInvoke(const ClientContextPtr& context, const std::string& url,
                                  const RequestMessage& req_msg, ResponseMessage* rsp_msg) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  context->SetReqEncodeType(serialization::kPbType);
  ConstructHttpRequestHeader(context, OperationType::POST, url);

  return RpcServiceProxy::UnaryInvoke<RequestMessage, ResponseMessage>(context, req_msg, rsp_msg);
}

template <class RequestMessage, class ResponseMessage>
Future<ResponseMessage> HttpServiceProxy::AsyncPBInvoke(const ClientContextPtr& context, const std::string& url,
                                                        const RequestMessage& req_msg) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  context->SetReqEncodeType(serialization::kPbType);
  ConstructHttpRequestHeader(context, OperationType::POST, url);

  return RpcServiceProxy::AsyncUnaryInvoke<RequestMessage, ResponseMessage>(context, req_msg);
}

template <class RequestMessage, class ResponseMessage>
Status HttpServiceProxy::UnaryInvoke(const ClientContextPtr& context, const RequestMessage& req, ResponseMessage* rsp) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }
  // Currently only supports PB data type.
  context->SetReqEncodeType(serialization::kPbType);
  ConstructRpcHttpRequestHeader(context);

  return RpcServiceProxy::UnaryInvoke<RequestMessage, ResponseMessage>(context, req, rsp);
}

template <class RequestMessage, class ResponseMessage>
Future<ResponseMessage> HttpServiceProxy::AsyncUnaryInvoke(const ClientContextPtr& context, const RequestMessage& req) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }
  // Currently only supports PB data type.
  context->SetReqEncodeType(serialization::kPbType);
  ConstructRpcHttpRequestHeader(context);

  return RpcServiceProxy::AsyncUnaryInvoke<RequestMessage, ResponseMessage>(context, req);
}

template <class RequestMessage, class ResponseMessage>
Status HttpServiceProxy::HttpUnaryInvoke(const ClientContextPtr& context, const RequestMessage& req,
                                         ResponseMessage* rsp) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req_msg = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req_msg)) {
    return Status(static_cast<int>(codec::ClientRetCode::ENCODE_ERROR), "unmatched codec");
  }
  *req_msg->request = req;

  // Pass the request method to the transport. Response content is not required for HTTP HEAD request.
  context->SetCurrentContextExt(static_cast<uint32_t>(req_msg->request->GetMethodType()));

  FillClientContext(context);

  ProtocolPtr& rsp_protocol = context->GetResponse();

  InnerUnaryInvoke(context, req_protocol, rsp_protocol);

  if (context->GetStatus().OK()) {
    auto* ret_rsp = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
    *rsp = std::move(ret_rsp->response);
  }
  return context->GetStatus();
}

template <class RequestMessage, class ResponseMessage>
Future<ResponseMessage> HttpServiceProxy::AsyncHttpUnaryInvoke(const ClientContextPtr& context,
                                                               const RequestMessage& req) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  ProtocolPtr& req_protocol = context->GetRequest();
  auto* req_msg = dynamic_cast<HttpRequestProtocol*>(req_protocol.get());
  // The conversion failed because `req_protocol` was created by the wrong codec.
  if (TRPC_UNLIKELY(!req_msg)) {
    return MakeExceptionFuture<ResponseMessage>(CommonException("unmatched codec"));
  }
  *req_msg->request = req;

  // Pass the request method to the transport. Response content is not required for HTTP HEAD request.
  context->SetCurrentContextExt(static_cast<uint32_t>(req_msg->request->GetMethodType()));

  FillClientContext(context);

  return AsyncInnerUnaryInvoke(context, req_protocol).Then([context](Future<ProtocolPtr>&& rsp_fut) {
    if (!rsp_fut.IsFailed()) {
      context->SetResponse(rsp_fut.GetValue0());

      ProtocolPtr& rsp_protocol = context->GetResponse();
      auto* rsp = static_cast<HttpResponseProtocol*>(rsp_protocol.get());
      return MakeReadyFuture<ResponseMessage>(std::move(rsp->response));
    }
    return MakeExceptionFuture<ResponseMessage>(rsp_fut.GetException());
  });
}

}  // namespace trpc::http
