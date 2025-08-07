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

#include <string>
#include <utility>

#include "rapidjson/document.h"

#include "trpc/client/client_context.h"
#include "trpc/client/service_proxy.h"
#include "trpc/codec/client_codec.h"
#include "trpc/codec/client_codec_factory.h"
#include "trpc/codec/codec_helper.h"
#include "trpc/codec/protocol.h"
#include "trpc/common/future/future_utility.h"
#include "trpc/common/status.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_latch.h"
#include "trpc/naming/trpc_naming.h"
#include "trpc/naming/common/common_defs.h"
#include "trpc/runtime/init_runtime.h"
#include "trpc/serialization/serialization_factory.h"
#include "trpc/serialization/serialization_type.h"
#include "trpc/stream/stream.h"
#include "trpc/stream/stream_async.h"
#include "trpc/stream/stream_handler.h"
#include "trpc/util/flatbuffers/message_fbs.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"
#include "trpc/util/unique_id.h"

namespace trpc {

/// @brief Rpc service proxy.
class RpcServiceProxy : public ServiceProxy {
 public:
  /// @brief Unary synchronous call, used by the upper-level user for input/output with the user protocol body.
  template <class RequestMessage, class ResponseMessage>
  Status UnaryInvoke(const ClientContextPtr& context, const RequestMessage& req, ResponseMessage* rsp);

  /// @brief Broadcast unary synchronous call, used by the upper-level user for input/output with the user protocol body.
  template <class RequestMessage, class ResponseMessage>
  Status BroadcastUnaryInvoke(const ClientContextPtr& broadcast_context, const RequestMessage& req,
                              std::vector<std::tuple<Status, ResponseMessage>>* rsp);

  /// @brief Unary asynchronous call, used by the upper-level user for input/output with the user protocol body.
  template <class RequestMessage, class ResponseMessage>
  Future<ResponseMessage> AsyncUnaryInvoke(const ClientContextPtr& context, const RequestMessage& req);

  /// @brief Broadcast unary asynchronous call, used by the upper-level user for input/output with the user protocol body.
  template <class RequestMessage, class ResponseMessage>
  Future<::trpc::Status, std::vector<std::tuple<::trpc::Status, ResponseMessage>>> AsyncBroadcastUnaryInvoke(
      const ClientContextPtr& broadcast_context, const RequestMessage& req);

  /// @brief One way call, used by the upper-level user for input with the user protocol body.
  template <class RequestProtocol>
  Status OnewayInvoke(const ClientContextPtr& context, const RequestProtocol& req);

  /// @brief Broadcast one way call, used by the upper-level user for input with the user protocol body.
  template <class RequestMessage>
  Status BroadcastOnewayInvoke(const ClientContextPtr& broadcast_context, const RequestMessage& req);

  /// @brief Unary synchronous call with NoncontiguousBuffer input parameter and pb output parameter.
  /// @param[in] req The NoncontiguousBuffer after PB serialization.
  /// @param[out] rsp The pb message.
  /// @note Use case: It is desirable to reuse the same request data structure each time when make a request, while only
  ///       modifying certain fields (thus avoiding duplicate copying). In this case, it is allowed to pass in a
  ///       serialized buffer as the request, so that the structure can be modified during the call. Then call this
  ///       interface after serializing the structure into binary data.
  Status PbSerializedReqUnaryInvoke(const ClientContextPtr& context, NoncontiguousBuffer&& req,
                                    google::protobuf::Message* rsp);

  /// @brief Server streaming interface, where the client sends a request message and receives a stream of response
  ///        messages from the server.
  template <class RequestMessage, class ResponseMessage>
  stream::StreamReader<ResponseMessage> StreamInvoke(const ClientContextPtr& context, const RequestMessage& request);

  /// @brief Client Streaming interface, where the client sends a stream of request messages and receives a response
  /// message
  ///        from the server.
  template <class RequestMessage, class ResponseMessage>
  stream::StreamWriter<RequestMessage> StreamInvoke(const ClientContextPtr& context, ResponseMessage* response);

  /// @brief Bidirectional Stream inferace, where the client sends a stream of request messages and receives a stream of
  ///        response messages from the server.
  template <class RequestMessage, class ResponseMessage>
  stream::StreamReaderWriter<RequestMessage, ResponseMessage> StreamInvoke(const ClientContextPtr& context);

  /// @brief Async client streaming interface.
  template <class W, class R>
  Future<std::pair<stream::AsyncWriterPtr<W>, Future<R>>> AsyncStreamInvoke(const ClientContextPtr& context);

  /// @brief Async server streaming interface.
  template <class W, class R>
  Future<stream::AsyncReaderPtr<R>> AsyncStreamInvoke(const ClientContextPtr& context, W&& request);

  /// @brief Async bidirectional Stream inferace.
  template <class W, class R>
  Future<stream::AsyncReaderWriterPtr<R, W>> CreateAsyncStream(const ClientContextPtr& context);

 private:
  template <class RequestMessage, class ResponseMessage>
  void UnaryInvokeImp(const ClientContextPtr& context, const RequestMessage& req, ResponseMessage* rsp);

  template <class RequestMessage, class ResponseMessage>
  Future<ResponseMessage> AsyncUnaryInvokeImp(const ClientContextPtr& context, const RequestMessage& req);

  // Select the default EncodeType and EncodeDataType base on the message type.
  // Note: The default matching can only be done for general types, and if it is a unknown user-defined type, the
  // EncodeType needs to be set explicitly.
  template <class Message>
  bool SelectDefalutEncodeType(serialization::SerializationType& encode_type, serialization::DataType& data_type) {
    if constexpr (std::is_convertible_v<Message*, google::protobuf::MessageLite*>) {
      data_type = serialization::kPbMessage;
      encode_type = serialization::kPbType;
    } else if constexpr (std::is_convertible_v<Message*, flatbuffers::trpc::MessageFbs*>) {
      data_type = serialization::kFlatBuffers;
      encode_type = serialization::kFlatBuffersType;
    } else if constexpr (std::is_convertible_v<Message*, rapidjson::Document*>) {
      data_type = serialization::kRapidJson;
      encode_type = serialization::kJsonType;
    } else if constexpr (std::is_convertible_v<Message*, std::string*>) {
      data_type = serialization::kStringNoop;
      encode_type = serialization::kNoopType;
    } else if constexpr (std::is_convertible_v<Message*, NoncontiguousBuffer*>) {
      data_type = serialization::kNonContiguousBufferNoop;
      encode_type = serialization::kNoopType;
    } else {
      return false;
    }

    return true;
  }

  template <class RequestMessage>
  bool SetReqEncode(const ClientContextPtr& context) {
    if (TRPC_UNLIKELY(context->IsTransparent())) {
      return true;
    }

    serialization::DataType selected_data_type;
    serialization::SerializationType selected_encode_type;
    if (!SelectDefalutEncodeType<RequestMessage>(selected_encode_type, selected_data_type)) {
      return false;
    }

    context->SetReqEncodeDataType(selected_data_type);
    // If the serialization type is not set in the context (default is PB type), the specific serialization type is
    // inferred based on the type of the request body.
    if (context->GetReqEncodeType() == serialization::kPbType) {
      context->SetReqEncodeType(selected_encode_type);
    }

    return true;
  }

  template <class ResponseMessage>
  bool SetRspEncode(const ClientContextPtr& context) {
    if (TRPC_UNLIKELY(context->IsTransparent())) {
      return true;
    }

    serialization::DataType selected_data_type;
    serialization::SerializationType selected_encode_type;
    if (!SelectDefalutEncodeType<ResponseMessage>(selected_encode_type, selected_data_type)) {
      return false;
    }

    context->SetRspEncodeDataType(selected_data_type);
    if (context->GetRspEncodeType() == serialization::kPbType) {
      context->SetRspEncodeType(selected_encode_type);
    }

    return true;
  }

  template <class RequestMessage, class ResponseMessage>
  bool SetMessageEncodeType(const ClientContextPtr& context) {
    if (!SetReqEncode<RequestMessage>(context) || !SetRspEncode<ResponseMessage>(context)) {
      std::string error("not support Message type.");
      context->SetStatus(Status(TrpcRetCode::TRPC_CLIENT_ENCODE_ERR, 0, std::move(error)));
      return false;
    }

    return true;
  }

  template <class W, class R>
  stream::StreamReaderWriter<W, R> CreateStreamReaderWriter(const ClientContextPtr& context, void* rpc_reply_msg);
};

template <class RequestMessage, class ResponseMessage>
Status RpcServiceProxy::UnaryInvoke(const ClientContextPtr& context, const RequestMessage& req, ResponseMessage* rsp) {
  TRPC_ASSERT(context->GetRequest() != nullptr);

  // Set the corresponding encoding based on the types of the request and response.
  if (TRPC_UNLIKELY(!(SetMessageEncodeType<RequestMessage, ResponseMessage>)(context))) {
    return context->GetStatus();
  }

  FillClientContext(context);

  // Set user request data struct
  context->SetRequestData(const_cast<RequestMessage*>(&req));
  // Set user response data struct
  context->SetResponseData(rsp);

  // Execute pre-RPC invoke filtes
  int filter_ret = RunFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_ret == 0) {
    UnaryInvokeImp<RequestMessage, ResponseMessage>(context, req, rsp);
  }
  // Execute post-RPC invoke filtes
  RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);

  context->SetRequestData(nullptr);
  context->SetResponseData(nullptr);

  context->SetEndTimestampUs(trpc::time::GetMicroSeconds());

  return context->GetStatus();
}

template <class RequestMessage, class ResponseMessage>
void RpcServiceProxy::UnaryInvokeImp(const ClientContextPtr& context, const RequestMessage& req, ResponseMessage* rsp) {
  const ProtocolPtr& req_protocol = context->GetRequest();
  if (!codec_->FillRequest(context, req_protocol, reinterpret_cast<void*>(const_cast<RequestMessage*>(&req)))) {
    std::string error("service name:");
    error += GetServiceName();
    error += ", request fill body failed.";
    TRPC_LOG_ERROR(error);

    Status status;
    status.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_ENCODE_ERR);
    status.SetErrorMessage(error);

    context->SetStatus(std::move(status));

    return;
  }

  ProtocolPtr& rsp_protocol = context->GetResponse();
  Status unary_invoke_status = ServiceProxy::UnaryInvoke(context, req_protocol, rsp_protocol);

  if (TRPC_UNLIKELY(!unary_invoke_status.OK())) {
    return;
  }

  if (!codec_->FillResponse(context, rsp_protocol, static_cast<void*>(rsp))) {
    std::string error("service name:");
    error += GetServiceName();
    error += ", response fill body failed.";
    TRPC_LOG_TRACE(error);

    if (context->GetStatus().OK()) {
      Status status;
      status.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_DECODE_ERR);
      status.SetErrorMessage(error);
      context->SetStatus(std::move(status));
    }
  }
}

template <class RequestMessage, class ResponseMessage>
Status RpcServiceProxy::BroadcastUnaryInvoke(const ClientContextPtr& broadcast_context, const RequestMessage& req,
                                             std::vector<std::tuple<Status, ResponseMessage>>* rsp) {
  TrpcSelectorInfo trpc_selector_info;
  // 构造广播类Selector路由相关信息并查询符合条件的节点
  MakeTrpcSelectorInfoForBroadcast(broadcast_context, trpc_selector_info);
  std::vector<TrpcEndpointInfo> endpoints;
  if (naming::SelectBatch(trpc_selector_info, endpoints) != 0) {
    TRPC_FMT_ERROR("BroadcastUnaryInvoke fail, error:SelectBatch failed.");
    return Status(-1, "SelectBatch failed");
  }

  if (endpoints.empty()) {
    TRPC_FMT_ERROR("BroadcastUnaryInvoke fail, error:SelectBatch get empty endpoint.");
    return Status(-1, "SelectBatch get empty endpoint");
  }

  rsp->clear();
  rsp->reserve(endpoints.size());

  if (runtime::IsInFiberRuntime()) {
    // Fiber 模式：并发访问每个下游节点并收集结果，最后汇总统一返回
    FiberLatch fiber_latch(endpoints.size() - 1);
    FiberMutex fiber_mutex;
    // 遍历前endpoints.size() - 1个节点发送请求
    for (size_t i = 0; i < endpoints.size() - 1; i++) {
      bool start_fiber = StartFiberDetached([&] {
        ClientContextPtr rpc_context = MakeRefCounted<ClientContext>();
        SetClientContextForBroadcast(broadcast_context, endpoints[i], rpc_context);
        ResponseMessage response;
        Status rpc_status = UnaryInvoke<RequestMessage, ResponseMessage>(rpc_context, req, &response);
        {
          // 加锁保护rsp
          std::unique_lock<FiberMutex> lk(fiber_mutex);
          rsp->emplace_back(std::make_tuple(rpc_status, std::move(response)));
        }

        fiber_latch.CountDown();
      });

      if (!start_fiber) {
        // 直接返回失败
        return Status(-1, "StartFiber failed");
      }
    }

    fiber_latch.Wait();

    // RPC最后一个节点在直接在当前Fiber执行
    broadcast_context->SetAddr(endpoints[endpoints.size() - 1].host, endpoints[endpoints.size() - 1].port);
    ResponseMessage response;
    Status rpc_status = UnaryInvoke<RequestMessage, ResponseMessage>(broadcast_context, req, &response);
    rsp->emplace_back(std::make_tuple(rpc_status, std::move(response)));

  } else {
    // 否则使用future模式，直接用UnaryInvoke(这里是串行，而非并行，因为Future不支持单向调用)
    // 遍历前endpoints.size() - 1个节点发送请求
    for (size_t i = 0; i < endpoints.size() - 1; i++) {
      ClientContextPtr rpc_context = MakeRefCounted<ClientContext>();
      SetClientContextForBroadcast(broadcast_context, endpoints[i], rpc_context);
      ResponseMessage response;
      Status rpc_status = UnaryInvoke<RequestMessage, ResponseMessage>(rpc_context, req, &response);
      rsp->emplace_back(std::make_tuple(rpc_status, std::move(response)));
    }

    // RPC最后一个节点复用broadcast_context
    broadcast_context->SetAddr(endpoints[endpoints.size() - 1].host, endpoints[endpoints.size() - 1].port);
    ResponseMessage response;
    Status rpc_status = UnaryInvoke<RequestMessage, ResponseMessage>(broadcast_context, req, &response);
    rsp->emplace_back(std::make_tuple(rpc_status, std::move(response)));
  }

  if (rsp->size() != endpoints.size()) {
    std::string error_message = "BroadcastUnaryInvoke fail, error: rsp->size():" + std::to_string(rsp->size()) +
                                " != endpoints.size():" + std::to_string(endpoints.size());
    return Status(-1, error_message);
  }

  // 整合最后结果
  ::trpc::Status boardcast_status;
  bool is_rpc_failed = false;
  // 获取所有节点的返回状态，有错误则设置broadcast_context状态
  std::string error_message = "";
  for (auto& item : *rsp) {
    ::trpc::Status rpc_status = std::get<0>(item);
    if (!rpc_status.OK()) {
      // 目前是将失败RPC 信息 追加到一起
      error_message.append("rpc peer endpoint failed err:");
      error_message.append(rpc_status.ToString());
      error_message.append("|");
      is_rpc_failed = true;
    }
  }

  if (is_rpc_failed == true) {
    boardcast_status.SetFuncRetCode(-1);
    boardcast_status.SetErrorMessage(error_message);
  }

  return boardcast_status;
}

template <class RequestMessage>
Status RpcServiceProxy::BroadcastOnewayInvoke(const ClientContextPtr& broadcast_context, const RequestMessage& req) {
  TrpcSelectorInfo trpc_selector_info;
  // 构造广播类Selector路由相关信息并查询符合条件的节点
  MakeTrpcSelectorInfoForBroadcast(broadcast_context, trpc_selector_info);
  std::vector<TrpcEndpointInfo> endpoints;
  if (naming::SelectBatch(trpc_selector_info, endpoints) != 0) {
    TRPC_FMT_ERROR("BroadcastOnewayInvoke fail, error:SelectBatch failed.");
    return Status(-1, "SelectBatch failed");
  }

  if (endpoints.empty()) {
    TRPC_FMT_ERROR("BroadcastOnewayInvoke fail, error:SelectBatch get empty endpoint.");
    return Status(-1, "SelectBatch get empty endpoint");
  }

  std::vector<Status> rsp;
  if (runtime::IsInFiberRuntime()) {
    // Fiber 模式：并发访问每个下游节点并收集结果，最后汇总统一返回
    FiberLatch fiber_latch(endpoints.size() - 1);
    FiberMutex fiber_mutex;
    // 遍历前endpoints.size() - 1个节点发送请求
    for (size_t i = 0; i < endpoints.size() - 1; i++) {
      bool start_fiber = StartFiberDetached([&] {
        ClientContextPtr rpc_context = MakeRefCounted<ClientContext>();
        SetClientContextForBroadcast(broadcast_context, endpoints[i], rpc_context);
        Status rpc_status = OnewayInvoke<RequestMessage>(rpc_context, req);
        {
          // 加锁保护rsp
          std::unique_lock<FiberMutex> lk(fiber_mutex);
          rsp.emplace_back(rpc_status);
        }

        fiber_latch.CountDown();
      });

      if (!start_fiber) {
        // 直接返回失败
        return Status(-1, "StartFiber failed");
      }
    }

    fiber_latch.Wait();

    // RPC最后一个节点在直接在当前Fiber执行
    broadcast_context->SetAddr(endpoints[endpoints.size() - 1].host, endpoints[endpoints.size() - 1].port);
    Status rpc_status = OnewayInvoke<RequestMessage>(broadcast_context, req);
    rsp.emplace_back(rpc_status);

  } else {
    // 否则使用future模式，直接用OnewayInvoke(这里是串行，而非并行，因为Future不支持单向调用)
    // 遍历前endpoints.size() - 1个节点发送请求
    for (size_t i = 0; i < endpoints.size() - 1; i++) {
      ClientContextPtr rpc_context = MakeRefCounted<ClientContext>();
      SetClientContextForBroadcast(broadcast_context, endpoints[i], rpc_context);
      rsp.emplace_back(OnewayInvoke<RequestMessage>(rpc_context, req));
    }

    // RPC最后一个节点复用broadcast_context
    broadcast_context->SetAddr(endpoints[endpoints.size() - 1].host, endpoints[endpoints.size() - 1].port);
    rsp.emplace_back(OnewayInvoke<RequestMessage>(broadcast_context, req));
  }

  if (rsp.size() != endpoints.size()) {
    std::string error_message = "BroadcastOnewayInvoke fail, error: rsp.size():" + std::to_string(rsp.size()) +
                                " != endpoints.size():" + std::to_string(endpoints.size());
    return Status(-1, error_message);
  }

  // 整合最后结果
  ::trpc::Status boardcast_status;
  bool is_rpc_failed = false;
  // 获取所有节点的返回状态，有错误则设置broadcast_context状态
  std::string error_message = "";
  for (auto& item : rsp) {
    if (!item.OK()) {
      // 目前是将失败RPC 信息 追加到一起
      error_message.append("rpc peer endpoint failed err:");
      error_message.append(item.ToString());
      error_message.append("|");
      is_rpc_failed = true;
    }
  }

  if (is_rpc_failed == true) {
    boardcast_status.SetFuncRetCode(-1);
    boardcast_status.SetErrorMessage(error_message);
  }

  return boardcast_status;
}

template <class RequestMessage, class ResponseMessage>
Future<ResponseMessage> RpcServiceProxy::AsyncUnaryInvoke(const ClientContextPtr& context, const RequestMessage& req) {
  TRPC_ASSERT(context->GetRequest() != nullptr);

  if (TRPC_UNLIKELY(!(SetMessageEncodeType<RequestMessage, ResponseMessage>)(context))) {
    return MakeExceptionFuture<ResponseMessage>(CommonException(
        context->GetStatus().ErrorMessage().c_str(), static_cast<int>(context->GetStatus().GetFrameworkRetCode())));
  }

  FillClientContext(context);

  context->SetRequestData(const_cast<RequestMessage*>(&req));

  int filter_ret = RunFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_ret == 0) {
    context->SetRequestData(nullptr);
    // Logic for execute post-RPC invoke filters is processed in AsyncUnaryTransportInvoke.
    // To reduce the number of layers of Future Then calls for performance optimization .
    return AsyncUnaryInvokeImp<RequestMessage, ResponseMessage>(context, req);
  }

  RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);

  context->SetRequestData(nullptr);
  context->SetEndTimestampUs(trpc::time::GetMicroSeconds());

  const Status& result = context->GetStatus();

  return MakeExceptionFuture<ResponseMessage>(CommonException(result.ErrorMessage().c_str()));
}

template <class RequestMessage, class ResponseMessage>
Future<ResponseMessage> RpcServiceProxy::AsyncUnaryInvokeImp(const ClientContextPtr& context,
                                                             const RequestMessage& req) {
  ProtocolPtr& req_protocol = context->GetRequest();
  if (!codec_->FillRequest(context, req_protocol, reinterpret_cast<void*>(const_cast<RequestMessage*>(&req)))) {
    std::string error("service name:");
    error += GetServiceName();
    error += ", request fill body failed.";

    TRPC_LOG_ERROR(error);

    Status status;
    status.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_ENCODE_ERR);
    status.SetErrorMessage(error);

    context->SetStatus(std::move(status));

    RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);

    context->SetEndTimestampUs(trpc::time::GetMicroSeconds());

    return MakeExceptionFuture<ResponseMessage>(
        CommonException(context->GetStatus().ErrorMessage().c_str(), TrpcRetCode::TRPC_CLIENT_ENCODE_ERR));
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

          TRPC_LOG_TRACE(error);

          if (context->GetStatus().OK()) {
            Status status;
            status.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_DECODE_ERR);
            status.SetErrorMessage(error);
            context->SetStatus(std::move(status));
          }
          RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);

          context->SetEndTimestampUs(trpc::time::GetMicroSeconds());

          return MakeExceptionFuture<ResponseMessage>(UnaryRpcError(context->GetStatus()));
        }

        // Set the response data (for use by the CLIENT_POST_RPC_INVOKE filter for instrumentation) when the call is
        // successful.
        context->SetResponseData(&rsp_obj);

        RunFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, context);

        context->SetResponseData(nullptr);
        context->SetEndTimestampUs(trpc::time::GetMicroSeconds());

        return MakeReadyFuture<ResponseMessage>(std::move(rsp_obj));
      });
}

template <class RequestMessage, class ResponseMessage>
Future<::trpc::Status, std::vector<std::tuple<::trpc::Status, ResponseMessage>>>
RpcServiceProxy::AsyncBroadcastUnaryInvoke(const ClientContextPtr& broadcast_context, const RequestMessage& req) {
  TrpcSelectorInfo trpc_selector_info;
  // 构造广播类Selector路由相关信息并查询符合条件的节点
  MakeTrpcSelectorInfoForBroadcast(broadcast_context, trpc_selector_info);
  std::vector<TrpcEndpointInfo> endpoints;

  return naming::AsyncSelectBatch(trpc_selector_info)
      .Then([this, broadcast_context, &req](Future<std::vector<TrpcEndpointInfo>> fut) {
        std::vector<std::tuple<::trpc::Status, ResponseMessage>> res;
        if (fut.IsFailed()) {
          TRPC_FMT_ERROR("AsyncBroadcastUnaryInvoke fail,error:AsyncSelectBatch naming select failed.");
          // 这里都是Ready，通过Status返回Status
          return MakeReadyFuture<::trpc::Status, std::vector<std::tuple<::trpc::Status, ResponseMessage>>>(
              Status(-1, "AsyncBroadcastUnaryInvoke fail,error:AsyncSelectBatch failed"), std::move(res));
        }

        std::vector<TrpcEndpointInfo> endpoints = fut.GetValue0();
        if (endpoints.empty()) {
          TRPC_FMT_ERROR("AsyncBroadcastUnaryInvoke fail,error:AsyncSelectBatch get empty endpoint.");
          // 这里都是Ready，通过Status返回Status
          return MakeReadyFuture<::trpc::Status, std::vector<std::tuple<::trpc::Status, ResponseMessage>>>(
              Status(-1, "AsyncBroadcastUnaryInvoke fail,error:SelectBatch get empty endpoint."), std::move(res));
        }

        std::vector<Future<ResponseMessage>> results;
        // 遍历前endpoints.size() - 1个节点发送请求
        for (size_t i = 0; i < endpoints.size() - 1; i++) {
          ClientContextPtr rpc_context = MakeRefCounted<ClientContext>();
          SetClientContextForBroadcast(broadcast_context, endpoints[i], rpc_context);
          Future<ResponseMessage> fut = AsyncUnaryInvoke<RequestMessage, ResponseMessage>(rpc_context, req);
          results.emplace_back(std::move(fut));
        }

        // RPC最后一个节点复用broadcast_context
        broadcast_context->SetAddr(endpoints[endpoints.size() - 1].host, endpoints[endpoints.size() - 1].port);
        results.emplace_back(std::move(AsyncUnaryInvoke<RequestMessage, ResponseMessage>(broadcast_context, req)));

        // whenall 并行访问
        return WhenAll(results.begin(), results.end())
            .Then([endpoints](std::vector<Future<ResponseMessage>>&& vec_futs) {
              std::vector<std::tuple<::trpc::Status, ResponseMessage>> res;
              for (auto& item : vec_futs) {
                if (item.IsReady()) {
                  res.emplace_back(std::make_tuple(kSuccStatus, item.GetValue0()));
                } else {
                  auto exception = item.GetException();
                  Status status;
                  status.SetFuncRetCode(exception.GetExceptionCode());
                  status.SetErrorMessage(exception.what());
                  res.emplace_back(std::make_tuple(status, ResponseMessage{}));
                }
              }

              if (res.size() != endpoints.size()) {
                std::string error_message =
                    "AsyncBroadcastUnaryInvoke fail, error: res.size():" + std::to_string(res.size()) +
                    " != endpoints.size():" + std::to_string(endpoints.size());
                return MakeReadyFuture<::trpc::Status, std::vector<std::tuple<::trpc::Status, ResponseMessage>>>(
                    Status(-1, error_message), std::move(res));
              }

              // 整合最后结果
              ::trpc::Status boardcast_status;
              bool is_rpc_failed = false;
              // 获取所有节点的返回状态，有错误则设置broadcast_context状态
              std::string error_message = "";
              for (auto& item : res) {
                ::trpc::Status rpc_status = std::get<0>(item);
                if (!rpc_status.OK()) {
                  // 目前是将失败RPC 信息 追加到一起
                  error_message.append("rpc peer endpoint failed err:");
                  error_message.append(rpc_status.ToString());
                  error_message.append("|");
                  is_rpc_failed = true;
                }
              }

              if (is_rpc_failed == true) {
                boardcast_status.SetFuncRetCode(-1);
                boardcast_status.SetErrorMessage(error_message);
              }

              return MakeReadyFuture<::trpc::Status, std::vector<std::tuple<::trpc::Status, ResponseMessage>>>(
                  boardcast_status, std::move(res));
            });
      });
}

template <class RequestProtocol>
Status RpcServiceProxy::OnewayInvoke(const ClientContextPtr& context, const RequestProtocol& req) {
  TRPC_ASSERT(context->GetRequest() != nullptr);

  context->SetCallType(kOnewayCall);

  if (TRPC_LIKELY(!SetReqEncode<RequestProtocol>(context))) {
    context->SetEndTimestampUs(trpc::time::GetMicroSeconds());

    std::string error("not support RequestMessage type.");
    Status status;
    status.SetFrameworkRetCode(static_cast<int>(TrpcRetCode::TRPC_CLIENT_ENCODE_ERR));
    status.SetErrorMessage(std::move(error));

    return status;
  }

  FillClientContext(context);

  context->SetRequestData(const_cast<RequestProtocol*>(&req));

  auto filter_ret = RunFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  if (filter_ret == 0) {
    const ProtocolPtr& req_protocol = context->GetRequest();

    if (!codec_->FillRequest(context, req_protocol, reinterpret_cast<void*>(const_cast<RequestProtocol*>(&req)))) {
      context->SetEndTimestampUs(trpc::time::GetMicroSeconds());

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
  context->SetEndTimestampUs(trpc::time::GetMicroSeconds());

  return context->GetStatus();
}

template <class RequestMessage, class ResponseMessage>
stream::StreamReader<ResponseMessage> RpcServiceProxy::StreamInvoke(const ClientContextPtr& context,
                                                                    const RequestMessage& request) {
  using W = RequestMessage;
  using R = ResponseMessage;

  // Create Stream for Reader/Writer
  auto reader_writer = CreateStreamReaderWriter<W, R>(context, nullptr);

  reader_writer.Write(request);

  return reader_writer.Reader();
}

template <class RequestMessage, class ResponseMessage>
stream::StreamWriter<RequestMessage> RpcServiceProxy::StreamInvoke(const ClientContextPtr& context,
                                                                   ResponseMessage* response) {
  using W = RequestMessage;
  using R = ResponseMessage;

  auto reader_writer = CreateStreamReaderWriter<W, R>(context, static_cast<void*>(response));

  return reader_writer.Writer();
}

template <class RequestMessage, class ResponseMessage>
stream::StreamReaderWriter<RequestMessage, ResponseMessage> RpcServiceProxy::StreamInvoke(
    const ClientContextPtr& context) {
  using W = RequestMessage;
  using R = ResponseMessage;

  return CreateStreamReaderWriter<W, R>(context, nullptr);
}

template <class W, class R>
stream::StreamReaderWriter<W, R> RpcServiceProxy::CreateStreamReaderWriter(const ClientContextPtr& context,
                                                                           void* rpc_reply_msg) {
  auto stream_provider = SelectStreamProvider(context, rpc_reply_msg);
  if (auto options = stream_provider->GetMutableStreamOptions(); options != nullptr) {
    options->stream_max_window_size = GetServiceProxyOption()->stream_max_window_size;
  }

  if (!SetReqEncode<W>(context)) {
    context->SetReqEncodeDataType(serialization::kMaxDataType);
    TRPC_FMT_ERROR("Not supported serialization type for request message");
  }
  if (!SetRspEncode<R>(context)) {
    context->SetRspEncodeDataType(serialization::kMaxDataType);
    TRPC_FMT_ERROR("Not supported serialization type for response message");
  }

  // Initialize the options for the message content encoder/decoder.
  stream::MessageContentCodecOptions content_codec_options{
      .serialization = serialization::SerializationFactory::GetInstance()->Get(context->GetReqEncodeType()),
      .content_type = context->GetReqEncodeType(),
  };

  stream_provider->Start();

  return stream::Create<W, R>(stream_provider, content_codec_options);
}

template <class W, class R>
Future<std::pair<stream::AsyncWriterPtr<W>, Future<R>>> RpcServiceProxy::AsyncStreamInvoke(
    const ClientContextPtr& context) {
  std::chrono::milliseconds timeout(context->GetTimeout());
  return CreateAsyncStream<W, R>(context).Then([timeout](stream::AsyncReaderWriterPtr<R, W>&& rw) {
    return MakeReadyFuture<std::pair<stream::AsyncWriterPtr<W>, Future<R>>>(
        std::make_pair(rw->Writer(), rw->Read(timeout).Then([](std::optional<R>&& reply) {
          if (reply)
            return MakeReadyFuture<R>(std::move(reply.value()));
          else
            return MakeExceptionFuture<R>(stream::StreamError(Status(-1, -1, "unexcepted stream eof")));
        })));
  });
}

template <class W, class R>
Future<stream::AsyncReaderPtr<R>> RpcServiceProxy::AsyncStreamInvoke(const ClientContextPtr& context, W&& request) {
  return CreateAsyncStream<W, R>(context).Then([request = std::move(request)](stream::AsyncReaderWriterPtr<R, W>&& rw) {
    return rw->Write(std::move(request)).Then([rw]() {
      return MakeReadyFuture<stream::AsyncReaderPtr<R>>(rw->Reader());
    });
  });
}

template <class W, class R>
Future<stream::AsyncReaderWriterPtr<R, W>> RpcServiceProxy::CreateAsyncStream(const ClientContextPtr& context) {
  auto stream = SelectStreamProvider(context);

  return stream->AsyncStart().Then([stream, context]() {
    stream::MessageContentCodecOptions content_codec_options{
        .serialization = serialization::SerializationFactory::GetInstance()->Get(context->GetReqEncodeType()),
        .content_type = context->GetReqEncodeType(),
    };

    auto rw = MakeRefCounted<stream::AsyncReaderWriter<R, W>>(stream, content_codec_options);
    return MakeReadyFuture<stream::AsyncReaderWriterPtr<R, W>>(std::move(rw));
  });
}

}  // namespace trpc
