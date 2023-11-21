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

#include "trpc/stream/trpc/trpc_server_stream.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/server/server_context.h"

namespace trpc::stream {

int TrpcServerStream::GetEncodeErrorCode() { return TrpcRetCode::TRPC_STREAM_SERVER_ENCODE_ERR; }

int TrpcServerStream::GetDecodeErrorCode() { return TrpcRetCode::TRPC_STREAM_SERVER_DECODE_ERR; }

int TrpcServerStream::GetReadTimeoutErrorCode() { return TrpcRetCode::TRPC_STREAM_SERVER_READ_TIMEOUT_ERR; }

Status TrpcServerStream::WriteDone() {
  TRPC_FMT_DEBUG("server stream writedone");
  return GetStatus();
}

Status TrpcServerStream::Start() {
  TRPC_FMT_DEBUG("server stream, start");
  // On the server side, the stream is passively opened and waits for the stream to be ready.
  return TrpcStream::Start();
}

Status TrpcServerStream::Finish() {
  // When on the server side, return the final status directly.
  TRPC_FMT_DEBUG("server stream finish");
  return GetStatus();
}

bool TrpcServerStream::CheckState(State state, Action action) {
  bool ok{true};
  std::string expected_state;

  switch (action) {
    case Action::kHandleInit:
      ok = state == State::kIdle;
      if (!ok) expected_state.append(StreamStateToString(State::kIdle));
      break;
    case Action::kSendInit:
      ok = state == State::kInit;
      if (!ok) expected_state.append(StreamStateToString(State::kInit));
      break;
    case Action::kHandleData:
      ok = state == State::kOpen || state == State::kLocalClosed;
      if (!ok)
        expected_state.append(StreamStateToString(State::kOpen))
            .append(" or ")
            .append(StreamStateToString(State::kLocalClosed));
      break;
    case Action::kSendData:
      ok = state == State::kOpen || state == State::kRemoteClosed;
      if (!ok)
        expected_state.append(StreamStateToString(State::kOpen))
            .append(" or ")
            .append(StreamStateToString(State::kRemoteClosed));
      break;
    case Action::kHandleClose:
      ok = state == State::kOpen;
      if (!ok) expected_state.append(StreamStateToString(State::kOpen));
      break;
    case Action::kSendClose:
      ok = state == State::kOpen || state == State::kRemoteClosed;
      if (!ok)
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

    Status status{TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR, 0, std::move(error_msg)};
    // Sends a Reset frame to the remote peer to indicate an exception.
    SendReset(status);
    // Pushes an error to the RPC interface of the business operation stream.
    OnError(status);
  }

  return ok;
}

RetCode TrpcServerStream::HandleInit(StreamRecvMessage&& msg) {
  TRPC_FMT_DEBUG("server stream, handle init begin, stream id: {}", GetId());

  if (TRPC_UNLIKELY(!CheckState(GetState(), Action::kHandleInit))) {
    return RetCode::kError;
  }

  auto* buffer = &std::any_cast<NoncontiguousBuffer&>(msg.message);
  TrpcStreamInitFrameProtocol init_frame{};
  if (TRPC_UNLIKELY(!DecodeMessage(buffer, &init_frame))) {
    return RetCode::kError;
  }

  const auto& context = std::any_cast<const ServerContextPtr&>(GetMutableStreamOptions()->context.context);
  const auto& request_metadata = init_frame.stream_init_metadata.request_meta();
  context->SetCallerName(request_metadata.caller());
  context->SetFuncName(request_metadata.func());
  context->SetReqEncodeType(init_frame.stream_init_metadata.content_type());
  context->GetMutablePbReqTransInfo()->insert(request_metadata.trans_info().begin(),
                                              request_metadata.trans_info().end());

  std::string var_path = "trpc/stream_rpc/server" + request_metadata.func();
  auto stream_var = StreamVarHelper::GetInstance()->GetOrCreateStreamVar(var_path);
  stream_var->AddRpcCallCount(1);
  SetStreamVar(std::move(stream_var));

  SetState(State::kInit);

  uint32_t send_window_size = init_frame.stream_init_metadata.init_window_size();
  // To be consistent with the tRPC-Go logic, flow control is only enabled when both ends have flow control enabled
  // (compatible with tRPC in other languages that do not implement flow control, where their window value is always 0).
  if (GetMutableStreamOptions()->fiber_mode && (send_window_size != 0)) {
    send_flow_controller_ = MakeRefCounted<TrpcStreamSendController>(send_window_size);
  }

  PushSendMessage(StreamSendMessage{.category = StreamMessageCategory::kStreamInit});

  TRPC_FMT_DEBUG("server stream, handle init end, stream id: {}", GetId());

  return RetCode::kSuccess;
}

RetCode TrpcServerStream::HandleData(StreamRecvMessage&& msg) {
  TRPC_FMT_DEBUG("server stream, handle data begin, stream id: {}", GetId());

  if (TRPC_UNLIKELY(!CheckState(GetState(), Action::kHandleData))) {
    return RetCode::kError;
  }

  RetCode ret = TrpcStream::HandleData(std::move(msg));
  TRPC_FMT_DEBUG("server stream, handle data end, stream id: {}", GetId());

  return ret;
}

RetCode TrpcServerStream::HandleClose(StreamRecvMessage&& msg) {
  TRPC_FMT_DEBUG("server stream, handle close begin, stream id: {}", GetId());

  if (GetState() == State::kClosed) {
    return RetCode::kSuccess;
  }

  auto* buffer = &std::any_cast<NoncontiguousBuffer&>(msg.message);
  TrpcStreamCloseFrameProtocol close_frame{};
  if (TRPC_UNLIKELY(!DecodeMessage(buffer, &close_frame))) {
    return RetCode::kError;
  }

  const TrpcStreamCloseMeta& close_metadata = close_frame.stream_close_metadata;
  bool reset = (close_metadata.close_type() == TrpcStreamCloseType::TRPC_STREAM_RESET);
  Status status{close_metadata.ret(), close_metadata.func_ret(), std::string(close_metadata.msg())};

  if (reset) {
    // When closing (RESET) due to an exception, there is no need to check the stream state.
    SetState(State::kClosed);
    if (status.OK()) {
      // Normally, the reset frame will carry error code. But if it doesn't exist, we add an unknown error.
      status.SetFrameworkRetCode(TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
      status.SetErrorMessage("stream reset recive, but unable to get error message");
    }
    OnError(status);
    return RetCode::kError;
  }

  // When closing stream normally, it needs to check the stream state.
  if (TRPC_UNLIKELY(!CheckState(GetState(), Action::kHandleClose))) {
    return RetCode::kError;
  }
  SetState(State::kRemoteClosed);
  // Notify with EOF.
  OnFinish(status);

  TRPC_FMT_DEBUG("server stream, handle close end, stream id: {}", GetId());

  return RetCode::kSuccess;
}

RetCode TrpcServerStream::SendInit(StreamSendMessage&& msg) {
  TRPC_FMT_DEBUG("server stream, send init begin, stream id: {}", GetId());

  if (TRPC_UNLIKELY(!CheckState(GetState(), Action::kSendInit))) {
    return RetCode::kError;
  }

  const auto& context = std::any_cast<const ServerContextPtr&>(GetMutableStreamOptions()->context.context);
  // Call RpcServiceImpl::DispatchStream().
  // This will check if the requested RPC method exists and set the context.status.
  STransportReqMsg req;
  req.context = context;
  GetMutableStreamOptions()->callbacks.handle_streaming_rpc_cb(&req);

  uint32_t recv_window_size = GetTrpcStreamWindowSize(GetMutableStreamOptions()->stream_max_window_size);
  if (GetMutableStreamOptions()->fiber_mode && (send_flow_controller_ != nullptr && recv_window_size != 0)) {
    recv_flow_controller_ = MakeRefCounted<TrpcStreamRecvController>(recv_window_size);
  } else {
    recv_window_size = 0;
    send_flow_controller_ = nullptr;
  }

  TrpcStreamInitFrameProtocol init_frame{};
  init_frame.fixed_header.stream_id = GetId();
  init_frame.stream_init_metadata.set_init_window_size(recv_window_size);
  auto* response_metadata = init_frame.stream_init_metadata.mutable_response_meta();

  bool stream_ok{true};
  // If handle_streaming_rpc_cb encounters an error, notify the closure of the stream and notify the client.
  // Currently, the main check is whether the requested RPC method exists.
  const auto status = context->GetStatus();
  if (!status.OK()) {
    response_metadata->set_ret(status.GetFrameworkRetCode());
    response_metadata->set_error_msg(status.ErrorMessage());
    stream_ok = false;
    TRPC_FMT_ERROR("bad status: {}, {}", status.GetFrameworkRetCode(), status.ErrorMessage());
  }
  init_frame.fixed_header.data_frame_size = init_frame.ByteSizeLong();

  NoncontiguousBuffer buffer;
  if (TRPC_UNLIKELY(!EncodeMessage(&init_frame, &buffer))) {
    return RetCode::kError;
  }

  if (TRPC_UNLIKELY(!Send(std::move(buffer)))) {
    return RetCode::kError;
  }

  if (TRPC_LIKELY(stream_ok)) {
    // After sending INIT, wait asynchronously for the INIT response.
    SetState(State::kOpen);
    OnReady();
  } else {
    SetState(State::kClosed);
    return RetCode::kError;
  }

  TRPC_FMT_DEBUG("server stream, send init end, stream id: {}", GetId());

  return RetCode::kSuccess;
}

RetCode TrpcServerStream::SendData(StreamSendMessage&& msg) {
  TRPC_FMT_DEBUG("server stream, send data begin, stream id: {}", GetId());

  if (TRPC_UNLIKELY(!CheckState(GetState(), Action::kSendData))) {
    return RetCode::kError;
  }

  RetCode ret = TrpcStream::SendData(std::move(msg));
  TRPC_FMT_DEBUG("server stream, send data end, stream id: {}", GetId());

  return ret;
}

RetCode TrpcServerStream::SendClose(StreamSendMessage&& msg) {
  TRPC_FMT_DEBUG("server stream, send close begin, stream id: {}", GetId());

  if (!TRPC_UNLIKELY(CheckState(GetState(), Action::kSendClose))) {
    return RetCode::kError;
  }

  const auto& context = std::any_cast<const ServerContextPtr&>(GetMutableStreamOptions()->context.context);

  auto& close_meta = std::any_cast<TrpcStreamCloseMeta&>(msg.metadata);
  close_meta.mutable_trans_info()->insert(context->GetPbRspTransInfo().begin(), context->GetPbRspTransInfo().end());

  RetCode ret = TrpcStream::SendClose(std::move(msg));
  if (TRPC_UNLIKELY(ret != RetCode::kSuccess)) {
    return ret;
  }

  SetState(State::kClosed);

  Stop();

  RunMessageFilter(FilterPoint::SERVER_PRE_SEND_MSG, context);

  TRPC_FMT_DEBUG("server stream, send close end, stream id: {}", GetId());
  return RetCode::kSuccess;
}

void TrpcServerStream::SendReset(const Status& status) {
  TrpcStream::SendReset(status);

  const auto& context = std::any_cast<const ServerContextPtr&>(GetMutableStreamOptions()->context.context);
  RunMessageFilter(FilterPoint::SERVER_PRE_SEND_MSG, context);
}

FilterStatus TrpcServerStream::RunMessageFilter(const FilterPoint& point, const ServerContextPtr& context) {
  if (context->HasFilterController()) {
    return context->GetFilterController().RunMessageServerFilters(point, context);
  }
  return FilterStatus::REJECT;
}

}  // namespace trpc::stream
