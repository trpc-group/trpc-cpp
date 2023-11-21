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

#include "trpc/stream/trpc/trpc_client_stream.h"
#include "trpc/client/client_context.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/serialization/serialization_factory.h"

namespace trpc::stream {

bool TrpcClientStream::CheckState(State state, Action action) {
  bool ok{true};
  std::string expected_state;
  switch (action) {
    case Action::kHandleInit:
      ok = state == State::kInit;
      expected_state.append(StreamStateToString(State::kInit));
      break;
    case Action::kSendInit:
      ok = state == State::kIdle;
      expected_state.append(StreamStateToString(State::kIdle));
      break;
    case Action::kHandleData:
      ok = state == State::kOpen || state == State::kLocalClosed;
      expected_state.append(StreamStateToString(State::kOpen))
          .append(" or ")
          .append(StreamStateToString(State::kLocalClosed));
      break;
    case Action::kSendData:
      ok = state == State::kOpen;
      expected_state.append(StreamStateToString(State::kOpen));
      break;
    case Action::kHandleClose:
      ok = state == State::kOpen || state == State::kLocalClosed;
      expected_state.append(StreamStateToString(State::kOpen))
          .append(" or ")
          .append(StreamStateToString(State::kLocalClosed));
      break;
    case Action::kSendClose:
      ok = state == State::kOpen;
      expected_state.append(StreamStateToString(State::kOpen));
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
    TRPC_LOG_ERROR(error_msg);

    Status status{TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR, 0, std::move(error_msg)};
    // Return a Reset from the other end to indicate an exception.
    SendReset(status);
    // Push error to the business operation flow's RPC interface.
    OnError(status);
  }

  return ok;
}

Status TrpcClientStream::WriteDone() {
  TRPC_FMT_DEBUG("client stream write done begin");
  auto error = CriticalSection<bool>(
      [this]() { return reader_writer_options_.simple_state == ReaderWriterOptions::SimpleState::Error; });
  if (error) return GetStatus();

  Close(kSuccStatus);

  if (GetMutableStreamOptions()->fiber_mode) {
    // Special note: this is special logic for the Client Streaming pattern, to read the response packets sent by the
    // server and set them to the caller.
    if (GetMutableStreamOptions()->rpc_reply_msg != nullptr) {
      NoncontiguousBuffer msg_buffer;
      auto status = Read(&msg_buffer, GetMutableStreamOptions()->stream_read_timeout);

      if (status.OK()) {
        TRPC_FMT_DEBUG("client stream writedone, read ok");
        bool ok = msg_codec_options_.serialization->Deserialize(&msg_buffer, msg_codec_options_.content_type,
                                                                GetMutableStreamOptions()->rpc_reply_msg);
        if (!ok) {
          Status status{GetDecodeErrorCode(), 0, "deserialize message failed"};
          TRPC_LOG_ERROR("client stream, " << status.ToString());
          return status;
        }
      }

      return status;
    }
  }

  TRPC_FMT_DEBUG("client stream writedone end witout rpc_reply_msg assigned");
  return GetStatus();
}

Status TrpcClientStream::Start() {
  TRPC_FMT_DEBUG("client stream, start");
  // The client actively opens the stream, first sending `INIT`, and then waiting for the stream to be ready.
  if (PushSendMessage(StreamSendMessage{.category = StreamMessageCategory::kStreamInit}) != RetCode::kSuccess)
    return GetStatus();
  return TrpcStream::Start();
}

Future<> TrpcClientStream::AsyncStart() {
  if (PushSendMessage(StreamSendMessage{.category = StreamMessageCategory::kStreamInit}) != RetCode::kSuccess)
    return MakeExceptionFuture<>(StreamError(GetStatus()));
  return TrpcStream::AsyncStart();
}

Status TrpcClientStream::Finish() {
  // No data needs to be sent, just wait for it to end.
  // When on the client side, wait for the final state of the stream.
  TRPC_FMT_DEBUG("client stream begin doing finish, stream id: {}", GetId());
  std::unique_lock lock(reader_writer_options_.mutex);
  bool no_timeout =
      reader_writer_options_.cond.wait_for(lock, GetTimeout(GetMutableStreamOptions()->stream_read_timeout), [this]() {
        return reader_writer_options_.simple_state == ReaderWriterOptions::SimpleState::Finished ||
               reader_writer_options_.simple_state == ReaderWriterOptions::SimpleState::Error;
      });
  lock.unlock();

  if (!no_timeout) {
    Status status{GetReadTimeoutErrorCode(), 0, std::move("wait for peer to finish timeout")};
    TRPC_LOG_ERROR("client stream, " << status.ToString());
    OnError(status);
    Stop();
    return status;
  }

  TRPC_FMT_DEBUG("client stream end doing finish, stream id: {}", GetId());
  return GetStatus();
}

Future<> TrpcClientStream::AsyncFinish() {
  if (GetState() != State::kClosed) {
    if (reader_writer_options_.simple_state == ReaderWriterOptions::Error) {
      Reset(GetStatus());
    } else {
      Close(GetStatus());
    }
  } else {
    // Sometimes the server will actively close the stream, which is also part of the normal process.
    // However, the client side does not have a `kRemoteClosed` state, so this operation is ignored directly
    // to maintain compatibility.
  }

  return TrpcStream::AsyncFinish();
}

RetCode TrpcClientStream::SendInit(StreamSendMessage&& msg) {
  TRPC_FMT_DEBUG("client stream, send init begin, stream id: {}", GetId());

  const auto& context = std::any_cast<const ClientContextPtr&>(GetMutableStreamOptions()->context.context);
  RunMessageFilter(FilterPoint::CLIENT_PRE_RPC_INVOKE, context);

  // Check the stream status: `IDLE` -> `INIT`. Generally, a stream only needs to send `INIT` once.
  if (TRPC_UNLIKELY(!CheckState(GetState(), Action::kSendInit))) {
    return RetCode::kError;
  }

  std::string var_path = "trpc/stream_rpc/client" + context->GetFuncName();
  auto stream_var = StreamVarHelper::GetInstance()->GetOrCreateStreamVar(var_path);
  stream_var->AddRpcCallCount(1);
  SetStreamVar(std::move(stream_var));

  msg_codec_options_ = stream::MessageContentCodecOptions{
      .serialization = serialization::SerializationFactory::GetInstance()->Get(context->GetReqEncodeType()),
      .content_type = context->GetReqEncodeDataType(),
  };

  TrpcStreamInitMeta stream_init_metadata{};
  stream_init_metadata.set_content_type(context->GetReqEncodeType());

  TrpcStreamInitRequestMeta* request_meta = stream_init_metadata.mutable_request_meta();
  request_meta->set_caller(context->GetCallerName());
  request_meta->set_callee(context->GetCalleeName());
  request_meta->set_func(context->GetFuncName());
  request_meta->set_message_type(context->GetMessageType());

  auto request_trans_info = request_meta->mutable_trans_info();
  request_trans_info->insert(context->GetPbReqTransInfo().begin(), context->GetPbReqTransInfo().end());
  if (!(context->GetDyeingKey().empty())) {
    request_trans_info->insert({"trpc_dyeing", context->GetDyeingKey()});
    request_meta->set_message_type(TrpcMessageType::TRPC_DYEING_MESSAGE);
  }

  if (GetMutableStreamOptions()->fiber_mode) {
    stream_init_metadata.set_init_window_size(
        GetTrpcStreamWindowSize(GetMutableStreamOptions()->stream_max_window_size));
  } else {
    stream_init_metadata.set_init_window_size(0);
  }
  stream_init_metadata.set_content_type(context->GetReqEncodeType());
  stream_init_metadata.set_content_encoding(TrpcCompressType::TRPC_DEFAULT_COMPRESS);

  TrpcStreamInitFrameProtocol init_frame{};
  init_frame.fixed_header.stream_id = GetId();
  init_frame.stream_init_metadata = std::move(stream_init_metadata);
  init_frame.fixed_header.data_frame_size = init_frame.ByteSizeLong();

  NoncontiguousBuffer buffer;
  if (TRPC_UNLIKELY(!EncodeMessage(&init_frame, &buffer))) {
    return RetCode::kError;
  }

  if (TRPC_UNLIKELY(!Send(std::move(buffer)))) {
    return RetCode::kError;
  }

  // After sending `INIT`, wait asynchronously for the `INIT` response.
  SetState(State::kInit);

  TRPC_FMT_DEBUG("client stream, send init end, stream id: {}", GetId());

  return RetCode::kSuccess;
}

RetCode TrpcClientStream::SendData(StreamSendMessage&& msg) {
  TRPC_FMT_DEBUG("client stream, send data begin, stream id: {}", GetId());

  if (TRPC_UNLIKELY(!CheckState(GetState(), Action::kSendData))) {
    return RetCode::kError;
  }

  RetCode ret = TrpcStream::SendData(std::move(msg));
  TRPC_FMT_DEBUG("client stream, send data end, stream id: {}", GetId());

  return ret;
}

RetCode TrpcClientStream::SendClose(StreamSendMessage&& msg) {
  TRPC_FMT_DEBUG("client stream, send close begin, stream id: {}", GetId());

  if (!TRPC_UNLIKELY(CheckState(GetState(), Action::kSendClose))) {
    return RetCode::kError;
  }

  RetCode ret = TrpcStream::SendClose(std::move(msg));
  if (TRPC_UNLIKELY(ret != RetCode::kSuccess)) {
    return ret;
  }

  SetState(State::kLocalClosed);
  TRPC_FMT_DEBUG("client stream, send close end, stream id: {}", GetId());

  return RetCode::kSuccess;
}

RetCode TrpcClientStream::HandleInit(StreamRecvMessage&& msg) {
  TRPC_FMT_DEBUG("client stream, handle init begin, stream id: {}", GetId());

  if (TRPC_UNLIKELY(!CheckState(GetState(), Action::kHandleInit))) {
    return RetCode::kError;
  }

  auto* buffer = &std::any_cast<NoncontiguousBuffer&>(msg.message);
  TrpcStreamInitFrameProtocol init_frame{};
  if (TRPC_UNLIKELY(!DecodeMessage(buffer, &init_frame))) {
    return RetCode::kError;
  }

  const auto& response_metadata = init_frame.stream_init_metadata.response_meta();
  if (TRPC_UNLIKELY(response_metadata.ret() != TrpcRetCode::TRPC_INVOKE_SUCCESS)) {
    Status status{response_metadata.ret(), 0, std::string(response_metadata.error_msg())};
    TRPC_LOG_ERROR(status.ToString());
    SetState(TrpcStream::State::kClosed);
    OnError(status);
    return RetCode::kError;
  }

  SetState(TrpcStream::State::kOpen);

  uint32_t send_window_size = init_frame.stream_init_metadata.init_window_size();
  uint32_t recv_window_size = GetTrpcStreamWindowSize(GetMutableStreamOptions()->stream_max_window_size);
  // Consistent with tRPC-Go logic, flow control is only enabled if both ends have enabled flow control (for
  // compatibility with other tRPC languages that do not implement flow control, their window size is always 0).
  if (GetMutableStreamOptions()->fiber_mode && (send_window_size != 0 && recv_window_size != 0)) {
    send_flow_controller_ = MakeRefCounted<TrpcStreamSendController>(send_window_size);
    recv_flow_controller_ = MakeRefCounted<TrpcStreamRecvController>(recv_window_size);
  }

  OnReady();

  TRPC_FMT_DEBUG("client stream, handle init end, stream id: {}", GetId());

  return RetCode::kSuccess;
}

RetCode TrpcClientStream::HandleData(StreamRecvMessage&& msg) {
  TRPC_FMT_DEBUG("client stream, handle data begin, stream id: {}", GetId());

  if (TRPC_UNLIKELY(!CheckState(GetState(), Action::kHandleData))) {
    return RetCode::kError;
  }

  RetCode ret = TrpcStream::HandleData(std::move(msg));
  TRPC_FMT_DEBUG("client stream, handle data end, stream id: {}", GetId());

  return ret;
}

RetCode TrpcClientStream::HandleClose(StreamRecvMessage&& msg) {
  TRPC_FMT_DEBUG("client stream, handle close begin, stream id: {}", GetId());

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

  const auto& context = std::any_cast<const ClientContextPtr&>(GetMutableStreamOptions()->context.context);
  context->SetRspTransInfo(close_metadata.trans_info().begin(), close_metadata.trans_info().end());

  if (reset) {
    // Close (RESET) due to an exception, so there is no need to check the stream status.
    // Notify the business that the stream has failed.
    SetState(State::kClosed);
    if (status.OK()) {
      // Normally, the reset frame will carry error code. But if it doesn't exist, we add an unknown error.
      status.SetFrameworkRetCode(TrpcRetCode::TRPC_STREAM_UNKNOWN_ERR);
      status.SetErrorMessage("stream reset recive, but unable to get error message");
    }
    OnError(status);
    return RetCode::kError;
  }

  // Close normally, and need to check that the stream is in the correct state.
  if (TRPC_UNLIKELY(!CheckState(GetState(), Action::kHandleClose))) {
    return RetCode::kError;
  }
  SetState(State::kClosed);

  // Check if an error has occurred (error information is carried when operating the stream through the interface).
  // Only framework error codes need to be pushed with errors; otherwise, the `EOF` should be pushed.
  if (TRPC_UNLIKELY(status.GetFrameworkRetCode() != TrpcRetCode::TRPC_INVOKE_SUCCESS)) {
    TRPC_LOG_ERROR(status.ErrorMessage());
    OnError(status);
    return RetCode::kError;
  }

  OnFinish(status);
  Stop();
  RunMessageFilter(FilterPoint::CLIENT_POST_RPC_INVOKE, context);

  TRPC_FMT_DEBUG("client stream, handle close end, stream id: {}", GetId());
  return RetCode::kSuccess;
}

int TrpcClientStream::GetEncodeErrorCode() { return TrpcRetCode::TRPC_STREAM_CLIENT_ENCODE_ERR; }

int TrpcClientStream::GetDecodeErrorCode() { return TrpcRetCode::TRPC_STREAM_CLIENT_DECODE_ERR; }

int TrpcClientStream::GetReadTimeoutErrorCode() { return TrpcRetCode::TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR; }

void TrpcClientStream::SendReset(const Status& status) {
  TrpcStream::SendReset(status);

  const auto& context = std::any_cast<const ClientContextPtr&>(GetMutableStreamOptions()->context.context);
  RunMessageFilter(FilterPoint::CLIENT_POST_RPC_INVOKE, context);
}

FilterStatus TrpcClientStream::RunMessageFilter(const FilterPoint& point, const ClientContextPtr& context) {
  if (filter_controller_) {
    return filter_controller_->RunMessageClientFilters(point, context);
  }
  return FilterStatus::REJECT;
}

}  // namespace trpc::stream
