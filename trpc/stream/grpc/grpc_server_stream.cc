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

#include "trpc/stream/grpc/grpc_server_stream.h"

#include "trpc/codec/grpc/grpc_protocol.h"
#include "trpc/codec/grpc/grpc_stream_frame.h"
#include "trpc/server/method.h"
#include "trpc/server/server_context.h"
#include "trpc/server/service.h"
#include "trpc/stream/grpc/grpc_stream.h"
#include "trpc/util/time.h"

namespace trpc::stream {

Status GrpcServerStream::Write(NoncontiguousBuffer&& msg) {
  auto status = GetStatus();
  if (status.OK()) {
    do {
      std::unique_lock lk(*mutex_);
      if (!session_->DecreaseRemoteWindowSize(msg.ByteSize())) {
        TRPC_FMT_INFO_IF(TRPC_EVERY_N(1000), "Flow control effect");
        cv_->wait_for(lk, std::chrono::milliseconds(write_flow_control_timeout_));
      } else {
        lk.unlock();
        break;
      }
      status = GetStatus();
    } while (status.OK());
    return CommonStream::Write(std::move(msg));
  }
  return status;
}

Status GrpcServerStream::WriteDone() {
  TRPC_FMT_DEBUG("server stream writedone");
  return GetStatus();
}

Status GrpcServerStream::Start() {
  // When on the server side, return the final status directly.
  TRPC_FMT_DEBUG("server stream, start");
  return GrpcStream::Start();
}

Status GrpcServerStream::Finish() {
  // When on the server side, return the final status directly.
  TRPC_FMT_DEBUG("server stream, finish");
  return GetStatus();
}

bool GrpcServerStream::CheckState(State state, Action action) {
  bool ok{true};
  std::string expected_state;

  switch (action) {
    case Action::kHandleInit:
      ok = state == State::kIdle;
      expected_state.append(StreamStateToString(State::kIdle));
      break;
    case Action::kHandleData:
    case Action::kHandleClose:
      ok = state == State::kOpen;
      expected_state.append(StreamStateToString(State::kOpen));
      break;
    case Action::kSendInit:
    case Action::kSendData:
    case Action::kSendClose:
      ok = state == State::kOpen || state == State::kRemoteClosed;
      expected_state.append(StreamStateToString(State::kOpen))
          .append(" or ")
          .append(StreamStateToString(State::kRemoteClosed));
      break;
    default:
      ok = false;
      expected_state.append("error state");
  }

  if (TRPC_UNLIKELY(!ok)) {
    std::string error_msg{"bad stream state, stream id: " + std::to_string(GetId())};
    error_msg.append(", action: ").append(StreamActionToString(action));
    error_msg.append(", stream state: ").append(StreamStateToString(GetState()));
    error_msg.append(", expected state: ").append(expected_state);
    TRPC_FMT_ERROR(error_msg);

    Status status{GrpcStatus::kGrpcInternal, 0, std::move(error_msg)};
    // Sends a Reset frame to the remote peer to indicate an exception.
    SendReset(GrpcStatus::kGrpcInternal);
    // Pushes an error to the RPC interface of the business operation stream.
    OnError(status);
  }

  return ok;
}

FiberStreamJobScheduler::RetCode GrpcServerStream::HandleInit(StreamRecvMessage&& msg) {
  TRPC_FMT_DEBUG("server stream, handle init begin, stream id: {}, conn_id: {}", GetId(),
                 GetMutableStreamOptions()->connection_id);

  if (TRPC_UNLIKELY(!CheckState(GetState(), Action::kHandleInit))) {
    return RetCode::kError;
  }

  auto frame = static_cast<GrpcStreamInitFrame*>(std::any_cast<GrpcStreamFramePtr&>(msg.message).get());
  http2::RequestPtr http2_request = frame->GetRequest();

  const auto& context = std::any_cast<const ServerContextPtr&>(GetMutableStreamOptions()->context.context);

  // Currently only supports serialization and decompression of PB data.
  // Set the protocol to allow the business RPC interface and process to obtain corresponding information.
  auto grpc_request = static_cast<GrpcUnaryRequestProtocol*>(context->GetRequestMsg().get());
  // Set FuncName.
  // Caller and Callee are not required in gRPC.
  grpc_request->SetFuncName(http2_request->GetPath());

  // Content-Type.
  std::string content_type{""};
  internal::GetHeaderString(http2_request->GetHeader(), kGrpcContentTypeName, &content_type);
  context->SetReqEncodeType(GrpcContentTypeToTrpcContentType(content_type));

  // Content-Encoding.
  std::string content_encoding{""};
  internal::GetHeaderString(http2_request->GetHeader(), kGrpcEncodingName, &content_encoding);
  context->SetReqCompressType(GrpcContentEncodingToTrpcContentEncoding(content_encoding));

  context->SetRspEncodeType(context->GetReqEncodeType());
  context->SetRspCompressType(context->GetReqCompressType());
  // Key-values defined by user.
  for (const auto& [name, value] : http2_request->GetHeaderPairs()) {
    auto token = http2::LookupToken(reinterpret_cast<const uint8_t*>(name.data()), name.size());
    // Filter out the request headers set by the user, exclude HTTP2-related headers, and exclude gRPC reserved headers.
    if (token == -1 && !http::StringStartsWithLiteralsIgnoreCase(name, "grpc-")) {
      grpc_request->SetKVInfo(std::string{name}, std::string{value});
    }
  }

  std::string var_path = "trpc/stream_rpc/server" + context->GetFuncName();
  auto stream_var = StreamVarHelper::GetInstance()->GetOrCreateStreamVar(var_path);
  stream_var->AddRpcCallCount(1);
  SetStreamVar(std::move(stream_var));

  // Call RpcServiceImpl::DispatchStream().
  // This will check if the requested RPC method exists and set the context.status.
  STransportReqMsg req;
  req.context = context;
  GetMutableStreamOptions()->callbacks.handle_streaming_rpc_cb(&req);

  const Status& status = context->GetStatus();
  if (TRPC_UNLIKELY(!status.OK())) {
    TRPC_FMT_ERROR("bad status: {} ", status.ToString());
    SendReset(GrpcStatus::kGrpcInternal);
    OnError(status);
    return RetCode::kError;
  }

  SetState(State::kOpen);

  OnReady();

  TRPC_FMT_DEBUG("server stream, handle init end, stream id: {}, conn_id: {}", GetId(),
                 GetMutableStreamOptions()->connection_id);

  return RetCode::kSuccess;
}

FiberStreamJobScheduler::RetCode GrpcServerStream::HandleData(StreamRecvMessage&& msg) {
  TRPC_FMT_DEBUG("server stream, handle data begin, stream id: {}, conn_id: {}", GetId(),
                 GetMutableStreamOptions()->connection_id);

  if (TRPC_UNLIKELY(!CheckState(GetState(), Action::kHandleData))) {
    return RetCode::kError;
  }

  auto frame = static_cast<GrpcStreamDataFrame*>(std::any_cast<GrpcStreamFramePtr&>(msg.message).get());
  auto data = frame->GetData();

  GrpcMessageContent content;

  if (TRPC_UNLIKELY(!content.Decode(&data))) {
    Status status{GetEncodeErrorCode(), 0, "Decode GrpcMessageContent failed."};
    TRPC_FMT_ERROR(status.ToString());
    SendReset(GrpcStatus::kGrpcInternal);
    OnError(status);
    return RetCode::kError;
  }

  // Compression is not currently supported.
  if (content.compressed) {
    Status status{GetEncodeErrorCode(), 0, "Compress is not currently supported in grpc-stream."};
    TRPC_FMT_ERROR(status.ToString());
    SendReset(GrpcStatus::kGrpcInternal);
    OnError(status);
    return RetCode::kError;
  }

  auto stream_var = GetStreamVar();
  if (stream_var) {
    size_t recv_bytes = content.content.ByteSize();
    stream_var->AddRecvMessageBytes(recv_bytes);
    stream_var->AddRecvMessageCount(1);
  }

  // Notify that a response message has been received and hand over the data to the application layer for processing.
  OnData(std::move(content.content));

  TRPC_FMT_DEBUG("server stream, handle data end, stream id: {}, conn_id: {}", GetId(),
                 GetMutableStreamOptions()->connection_id);

  return RetCode::kSuccess;
}

FiberStreamJobScheduler::RetCode GrpcServerStream::HandleClose(StreamRecvMessage&& msg) {
  TRPC_FMT_DEBUG("server stream, handle close begin, stream id: {}, conn_id: {}", GetId(),
                 GetMutableStreamOptions()->connection_id);

  if (GetState() == State::kClosed) {
    return RetCode::kSuccess;
  }

  // When handling server streaming, there is no need to handle EOF. The request has already ended after the first
  // Data frame is received.
  const auto& context = std::any_cast<const ServerContextPtr&>(GetMutableStreamOptions()->context.context);
  const auto& rpc_service_method = context->GetService()->GetRpcServiceMethod();
  // This method must be found here, because non-existent methods will follow unary RPC, and the unary logic checks
  // that the method does not exist and returns directly.
  auto it = rpc_service_method.find(context->GetFuncName());
  TRPC_ASSERT(it != rpc_service_method.end() && "method not found");
  if (it->second->GetMethodType() == trpc::MethodType::SERVER_STREAMING) {
    return RetCode::kSuccess;
  }

  // Normal closure, need to check that the stream is in the correct state.
  if (TRPC_UNLIKELY(!CheckState(GetState(), Action::kHandleClose))) {
    return RetCode::kError;
  }

  SetState(State::kRemoteClosed);

  // Notify the stream is ended. When the client actively closes the gRPC stream, there is no error code, so it
  // returns a successful status to the upper layer.
  OnFinish(kSuccStatus);

  TRPC_FMT_DEBUG("server stream, handle close end, stream id: {}, conn_id: {}", GetId(),
                 GetMutableStreamOptions()->connection_id);

  return RetCode::kSuccess;
}

FiberStreamJobScheduler::RetCode GrpcServerStream::SendInit(StreamSendMessage&& msg) {
  TRPC_FMT_DEBUG("server stream, send init begin, stream id: {}, conn_id: {}", GetId(),
                 GetMutableStreamOptions()->connection_id);

  if (TRPC_UNLIKELY(!CheckState(GetState(), Action::kSendInit))) {
    return RetCode::kError;
  }

  // Sends HEADER.
  http2::HeaderPairs header;
  header.Add(http2::kHttp2HeaderStatusName, http2::kHttp2HeaderStatusOk);
  // Compression is not supported for now.
  // Currently only supports PB type.
  header.Add(kGrpcContentTypeName, kGrpcContentTypeDefault);
  std::string date = TimeStringHelper::ConvertEpochToHttpDate(GetNowAsTimeT());
  header.Add(http2::kHttp2HeaderDateName, std::move(date));

  NoncontiguousBuffer buffer;
  if (TRPC_UNLIKELY(!EncodeMessage(EncodeMessageType::kHeader, std::move(header), &buffer))) {
    return RetCode::kError;
  }
  if (TRPC_UNLIKELY(!Send(std::move(buffer)))) {
    return RetCode::kError;
  }

  TRPC_FMT_DEBUG("server stream, send init end, stream id: {}, conn_id: {}", GetId(),
                 GetMutableStreamOptions()->connection_id);

  return RetCode::kSuccess;
}

FiberStreamJobScheduler::RetCode GrpcServerStream::SendData(StreamSendMessage&& msg) {
  TRPC_FMT_DEBUG("server stream, send data begin, stream id: {}, conn_id: {}", GetId(),
                 GetMutableStreamOptions()->connection_id);

  if (TRPC_UNLIKELY(!CheckState(GetState(), Action::kSendData))) {
    return RetCode::kError;
  }

  // Handling gRPC Data frames. gRPC only sends INIT frames when the first DATA frame is sent.
  if (!send_header_) {
    if (TRPC_UNLIKELY(SendInit(StreamSendMessage{.category = StreamMessageCategory::kStreamInit}) == RetCode::kError)) {
      return RetCode::kError;
    }
    send_header_ = true;
  }

  auto& data = std::any_cast<NoncontiguousBuffer&>(msg.data_provider);
  const auto& context = std::any_cast<const ServerContextPtr&>(GetMutableStreamOptions()->context.context);
  // Compression is not supported for now.
  if (context->GetRspCompressType() != TrpcCompressType::TRPC_DEFAULT_COMPRESS) {
    Status status{GetEncodeErrorCode(), 0, "compress currently not supported in grpc-streaming"};
    TRPC_FMT_ERROR(status.ToString());
    SendReset(status.GetFrameworkRetCode());
    OnError(status);
    return RetCode::kError;
  }

  GrpcMessageContent grpc_msg_content{};
  grpc_msg_content.compressed = kGrpcNotSupportCompressed;
  grpc_msg_content.content = std::move(std::move(data));
  grpc_msg_content.length = grpc_msg_content.content.ByteSize();

  NoncontiguousBuffer response_content;
  if (TRPC_UNLIKELY(!grpc_msg_content.Encode(&response_content))) {
    Status status{GetEncodeErrorCode(), 0, "encode grpc message content failed"};
    TRPC_FMT_ERROR(status.ToString());
    SendReset(status.GetFrameworkRetCode());
    OnError(status);
    return RetCode::kError;
  }

  NoncontiguousBuffer buffer;
  if (TRPC_UNLIKELY(!EncodeMessage(EncodeMessageType::kData, std::move(response_content), &buffer))) {
    return RetCode::kError;
  }

  if (TRPC_UNLIKELY(!Send(std::move(buffer)))) {
    return RetCode::kError;
  }

  if (GetStreamVar()) {
    GetStreamVar()->AddSendMessageBytes(grpc_msg_content.length);
    GetStreamVar()->AddSendMessageCount(1);
  }

  TRPC_FMT_DEBUG("server stream, send data end, stream id: {}, conn_id: {}", GetId(),
                 GetMutableStreamOptions()->connection_id);

  return RetCode::kSuccess;
}

FiberStreamJobScheduler::RetCode GrpcServerStream::SendClose(StreamSendMessage&& msg) {
  TRPC_FMT_DEBUG("server stream, send close begin, stream id: {}, conn_id: {}", GetId(),
                 GetMutableStreamOptions()->connection_id);

  if (!TRPC_UNLIKELY(CheckState(GetState(), Action::kSendClose))) {
    return RetCode::kError;
  }

  if (!send_header_) {
    if (SendInit(StreamSendMessage{.category = StreamMessageCategory::kStreamInit}) == RetCode::kError) {
      TRPC_FMT_DEBUG("server stream, send close send init failed, stream id: {}", GetId());
      return RetCode::kError;
    }
    send_header_ = true;
  }

  auto&& status = std::any_cast<Status&&>(std::move(msg.metadata));
  int grpc_status = 0;
  if (!status.OK()) {
    grpc_status = (status.GetFrameworkRetCode() != 0) ? status.GetFrameworkRetCode() : status.GetFuncRetCode();
  }

  http2::TrailerPairs trailer;
  trailer.Add(kGrpcStatusName, std::to_string(grpc_status));
  trailer.Add(kGrpcMessageName, status.ToString());

  NoncontiguousBuffer buffer;
  if (TRPC_UNLIKELY(!EncodeMessage(EncodeMessageType::kTrailer, std::move(trailer), &buffer))) {
    return RetCode::kError;
  }
  if (TRPC_UNLIKELY(!Send(std::move(buffer)))) {
    return RetCode::kError;
  }

  SetState(State::kClosed);

  TRPC_FMT_DEBUG("server stream, send close end, stream id: {}, conn_id: {}", GetId(),
                 GetMutableStreamOptions()->connection_id);

  Stop();

  const auto& context = std::any_cast<const ServerContextPtr&>(GetMutableStreamOptions()->context.context);
  RunMessageFilter(FilterPoint::SERVER_PRE_SEND_MSG, context);

  return RetCode::kSuccess;
}

void GrpcServerStream::SendReset(uint32_t error_code) {
  GrpcStream::SendReset(error_code);

  const auto& context = std::any_cast<const ServerContextPtr&>(GetMutableStreamOptions()->context.context);
  RunMessageFilter(FilterPoint::SERVER_PRE_SEND_MSG, context);
}

FilterStatus GrpcServerStream::RunMessageFilter(const FilterPoint& point, const ServerContextPtr& context) {
  if (context->HasFilterController()) {
    return context->GetFilterController().RunMessageServerFilters(point, context);
  }
  return FilterStatus::REJECT;
}

}  // namespace trpc::stream
