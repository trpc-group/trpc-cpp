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

#include <memory>
#include <string>
#include <utility>

#include "trpc/client/redis/formatter.h"
#include "trpc/client/redis/reply.h"
#include "trpc/client/redis/request.h"
#include "trpc/client/service_proxy.h"
#include "trpc/serialization/serialization_type.h"

namespace trpc {

namespace redis {

/// @brief Redis client proxy instance, supporting Redis Command such as set, get, mset, mget
class RedisServiceProxy : public ServiceProxy {
 public:
  RedisServiceProxy() : formatter_(new redis::Formatter()) {}

  ~RedisServiceProxy() override = default;

  /// @brief Redis synchronous call.
  /// @param context Client context
  /// @param reply is the response message.
  /// @param cmd is the redis command.
  /// @return Status Result of the interface execution; if successful, `Status::OK()` is true, otherwise it indicates
  /// that the call failed.
  Status Command(const ClientContextPtr& context, Reply* reply, const std::string& cmd);

  /// @brief Same as above interface which param cmd is right value
  Status Command(const ClientContextPtr& context, Reply* reply, std::string&& cmd);

  /// @brief Same as above interface which param cmd can be format style.
  /// For example:Command(context, reply, "get %s", usr_name.c_str())
  Status Command(const ClientContextPtr& context, Reply* reply, const char* format, ...);

  /// @brief Redis synchronous call.
  /// @param context Client context
  /// @param req is the redis request.
  /// @param reply is the response message.
  /// @return Status Result of the interface execution; if successful, `Status::OK()` is true, otherwise it indicates
  /// that the call failed.
  Status CommandArgv(const ClientContextPtr& context, const Request& req, Reply* reply);

  /// @brief Same as above interface which param req is right value
  Status CommandArgv(const ClientContextPtr& context, Request&& req, Reply* reply);

  /// @brief Redis asynchronous call.
  ///
  /// @param context Client context
  /// @param cmd is the redis command.
  /// @return Future<Reply> of the interface execution;
  Future<Reply> AsyncCommand(const ClientContextPtr& context, const std::string& cmd);

  /// @brief Same as above interface which param cmd is right value
  Future<Reply> AsyncCommand(const ClientContextPtr& context, std::string&& cmd);

  /// @brief Same as above interface which param cmd can be format style.
  /// For example:AsyncCommand(context, "get %s", usr_name.c_str())
  Future<Reply> AsyncCommand(const ClientContextPtr& context, const char* format, ...);

  /// @brief Redis asynchronous call.

  /// @param context Client context
  /// @param req is the redis request.
  /// @return Future<Reply> of the interface execution;
  Future<Reply> AsyncCommandArgv(const ClientContextPtr& context, const Request& req);

  /// @brief Same as above interface which param req is right value
  Future<Reply> AsyncCommandArgv(const ClientContextPtr& context, Request&& req);

  /// @brief Redis onway call.
  ///
  /// @param context Client context
  /// @param cmd is the redis command.
  /// @return Status Result of the interface execution; if successful, `Status::OK()` is true, otherwise it indicates
  /// that the call failed.
  Status Command(const ClientContextPtr& context, const std::string& cmd);

  /// @brief Same as above interface which param cmd is right value
  Status Command(const ClientContextPtr& context, std::string&& cmd);

 protected:
  /// @private For internal use purpose only.
  TransInfo ProxyOptionToTransInfo() override;

  /// @private For internal use purpose only.
  template <class RequestMessage, class ResponseMessage>
  Status UnaryInvoke(const ClientContextPtr& context, RequestMessage&& req, ResponseMessage* rsp);

  /// @private For internal use purpose only.
  template <class RequestMessage, class ResponseMessage>
  Future<ResponseMessage> AsyncUnaryInvoke(const ClientContextPtr& context, RequestMessage&& req);

  /// @private For internal use purpose only.
  template <class RequestMessage, class ResponseMessage>
  void UnaryInvokeImp(const ClientContextPtr& context, RequestMessage&& req, ResponseMessage* rsp);

  /// @private For internal use purpose only.
  template <class RequestMessage, class ResponseMessage>
  Future<ResponseMessage> AsyncUnaryInvokeImp(const ClientContextPtr& context, RequestMessage&& req);

  /// @private For internal use purpose only.
  template <class RequestMessage>
  Status OnewayInvoke(const ClientContextPtr& context, RequestMessage&& req);

 private:
  std::shared_ptr<redis::Formatter> formatter_;
};

template <class RequestMessage, class ResponseMessage>
Status RedisServiceProxy::UnaryInvoke(const ClientContextPtr& context, RequestMessage&& req, ResponseMessage* rsp) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  FillClientContext(context);

  context->SetRequestData(&req);
  context->SetReqEncodeDataType(serialization::kRedisType);

  context->SetRspEncodeType(serialization::kNoopType);
  context->SetRspEncodeDataType(serialization::kNoopType);

  auto filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);

  if (filter_status != FilterStatus::REJECT) {
    UnaryInvokeImp<RequestMessage, ResponseMessage>(context, std::move(req), rsp);
  } else {
    TRPC_FMT_ERROR("service name:{}, filter pre execute failed.", GetServiceName());
  }

  context->SetResponseData(rsp);

  filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);

  context->SetRequestData(nullptr);
  context->SetResponseData(nullptr);

  return context->GetStatus();
}

template <class RequestMessage, class ResponseMessage>
void RedisServiceProxy::UnaryInvokeImp(const ClientContextPtr& context, RequestMessage&& req, ResponseMessage* rsp) {
  const ProtocolPtr& req_protocol = context->GetRequest();

  if (!codec_->FillRequest(context, req_protocol, reinterpret_cast<void*>(&req))) {
    std::string error("service name:");
    error += GetServiceName();
    error += ", request fill body failed.";
    TRPC_LOG_ERROR(error);

    Status status;
    status.SetFrameworkRetCode(static_cast<int>(TrpcRetCode::TRPC_CLIENT_ENCODE_ERR));
    status.SetErrorMessage(error);

    context->SetStatus(std::move(status));
    return;
  }

  ProtocolPtr& rsp_protocol = context->GetResponse();

  Status status = ServiceProxy::UnaryInvoke(context, req_protocol, rsp_protocol);
  if (TRPC_UNLIKELY(!status.OK())) {
    return;
  }

  if (!codec_->FillResponse(context, rsp_protocol, reinterpret_cast<void*>(rsp))) {
    std::string error("service name:");
    error += GetServiceName();
    error += ", response fill body failed.";
    TRPC_LOG_ERROR(error);

    Status status;
    status.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_DECODE_ERR);
    status.SetErrorMessage(error);

    context->SetStatus(std::move(status));
    return;
  }
}

template <class RequestMessage, class ResponseMessage>
Future<ResponseMessage> RedisServiceProxy::AsyncUnaryInvoke(const ClientContextPtr& context, RequestMessage&& req) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  FillClientContext(context);

  context->SetRequestData(&req);
  context->SetReqEncodeDataType(serialization::kRedisType);

  context->SetRspEncodeType(serialization::kNoopType);
  context->SetRspEncodeDataType(serialization::kNoopType);

  auto filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_status == FilterStatus::REJECT) {
    TRPC_FMT_ERROR("service name:{}, filter execute failed.", GetServiceName());
    context->SetRequestData(nullptr);
    const Status& result = context->GetStatus();
    auto fut = MakeExceptionFuture<ResponseMessage>(CommonException(result.ErrorMessage().c_str()));
    filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
    return fut;
  }
  return AsyncUnaryInvokeImp<RequestMessage, ResponseMessage>(context, std::move(req));
}

template <class RequestMessage, class ResponseMessage>
Future<ResponseMessage> RedisServiceProxy::AsyncUnaryInvokeImp(const ClientContextPtr& context, RequestMessage&& req) {
  ProtocolPtr& req_protocol = context->GetRequest();

  if (!codec_->FillRequest(context, req_protocol, reinterpret_cast<void*>(&req))) {
    std::string error("service name:");
    error += GetServiceName();
    error += ", request fill body failed.";

    TRPC_LOG_ERROR(error);

    Status status;
    status.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_ENCODE_ERR);
    status.SetErrorMessage(error);

    context->SetStatus(std::move(status));

    RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);

    return MakeExceptionFuture<ResponseMessage>(CommonException(context->GetStatus().ErrorMessage().c_str()));
  }

  return ServiceProxy::AsyncUnaryInvoke(context, req_protocol)
      .Then([this, context](Future<ProtocolPtr>&& rsp_protocol) {
        if (rsp_protocol.IsFailed()) {
          RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
          return MakeExceptionFuture<ResponseMessage>(rsp_protocol.GetException());
        }

        context->SetResponse(rsp_protocol.GetValue0());

        ResponseMessage rsp_obj;
        void* raw_rsp = static_cast<void*>(&rsp_obj);

        if (!codec_->FillResponse(context, context->GetResponse(), raw_rsp)) {
          std::string error("service name:");
          error += GetServiceName();
          error += ", response fill body failed.";

          TRPC_LOG_ERROR(error);

          Status status;
          status.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_DECODE_ERR);
          status.SetErrorMessage(error);

          context->SetStatus(std::move(status));

          RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
          return MakeExceptionFuture<ResponseMessage>(CommonException(context->GetStatus().ErrorMessage().c_str()));
        }

        context->SetResponseData(&rsp_obj);

        RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
        return MakeReadyFuture<ResponseMessage>(std::move(rsp_obj));
      });
}

template <class RequestMessage>
Status RedisServiceProxy::OnewayInvoke(const ClientContextPtr& context, RequestMessage&& req) {
  if (!context->GetRequest()) {
    context->SetRequest(codec_->CreateRequestPtr());
  }

  context->SetRspEncodeType(serialization::kNoopType);
  context->SetRspEncodeDataType(serialization::kNoopType);
  context->SetCallType(kOnewayCall);

  FillClientContext(context);

  context->SetRequestData(&req);
  context->SetReqEncodeDataType(serialization::kRedisType);

  auto filter_ret = RunFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_ret == 0) {
    const ProtocolPtr& req_protocol = context->GetRequest();

    if (!codec_->FillRequest(context, req_protocol, reinterpret_cast<void*>(&req))) {
      std::string error("service name:");
      error += GetServiceName();
      error += ", request fill body failed.";
      TRPC_LOG_ERROR(error);

      Status status;
      status.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_ENCODE_ERR);
      status.SetErrorMessage(error);

      context->SetStatus(status);

      return status;
    }

    ServiceProxy::OnewayInvoke(context, req_protocol);
  }

  RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  context->SetRequestData(nullptr);
  return context->GetStatus();
}

}  // namespace redis

}  // namespace trpc
