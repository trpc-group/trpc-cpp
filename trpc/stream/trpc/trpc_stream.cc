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

#include "trpc/stream/trpc/trpc_stream.h"

#include "trpc/codec/trpc/trpc_protocol.h"

namespace trpc::stream {

namespace {

TrpcStreamCloseMeta ConstructTrpcStreamCloseMeta(const Status& status, bool reset) {
  TrpcStreamCloseMeta close_metadata{};

  close_metadata.set_close_type(reset ? TrpcStreamCloseType::TRPC_STREAM_RESET
                                      : TrpcStreamCloseType::TRPC_STREAM_CLOSE);
  close_metadata.set_ret(status.GetFrameworkRetCode());
  close_metadata.set_func_ret(status.GetFuncRetCode());
  close_metadata.set_msg(std::string(status.ErrorMessage()));

  return close_metadata;
}

}  // namespace

Status TrpcStream::Read(NoncontiguousBuffer* msg, int timeout) {
  Status read_status = CommonStream::Read(msg, timeout);
  if (recv_flow_controller_ != nullptr && read_status.OK()) {
    uint32_t window_increment = 0;
    {
      std::scoped_lock _(flow_control_mutex_);
      window_increment = recv_flow_controller_->UpdateConsumeBytes(msg->ByteSize());
    }
    if (window_increment != 0) {
      TrpcStreamFeedBackMeta feedback_meta;
      feedback_meta.set_window_size_increment(window_increment);
      PushSendMessage(StreamSendMessage{.category = StreamMessageCategory::kStreamFeedback,
                                        .data_provider = nullptr,
                                        .metadata = std::move(feedback_meta)},
                      true);
    }
  }
  return read_status;
}

Status TrpcStream::Write(NoncontiguousBuffer&& msg) {
  auto status = GetStatus();
  if (!status.OK()) {
    return status;
  }

  if (send_flow_controller_ != nullptr) {
    std::unique_lock lk(flow_control_mutex_);
    if (!send_flow_controller_->DecreaseWindow(msg.ByteSize())) {
      flow_control_cond_.wait(lk);
    }
  }
  return CommonStream::Write(std::move(msg));
}

void TrpcStream::OnError(Status status) {
  CommonStream::OnError(status);
  if (GetMutableStreamOptions()->fiber_mode && send_flow_controller_ != nullptr) {
    flow_control_cond_.notify_all();
  }
}

void TrpcStream::Close(Status status) {
  TRPC_FMT_DEBUG("stream, close stream begin, stream id: {}, status: {}", GetId(), status.ToString());
  TrpcStreamCloseMeta close_metadata = ConstructTrpcStreamCloseMeta(status, false);
  PushSendMessage(StreamSendMessage{.category = StreamMessageCategory::kStreamClose,
                                    .data_provider = nullptr,
                                    .metadata = std::move(close_metadata)});
  TRPC_FMT_DEBUG("stream, close stream end, stream id: {}, status: {}", GetId(), status.ToString());
}

void TrpcStream::Reset(Status status) {
  TRPC_FMT_DEBUG("stream, reset stream begin, stream id: {}, status: {}", GetId(), status.ToString());
  TrpcStreamCloseMeta close_metadata = ConstructTrpcStreamCloseMeta(status, true);
  PushSendMessage(StreamSendMessage{.category = StreamMessageCategory::kStreamClose,
                                    .data_provider = nullptr,
                                    .metadata = std::move(close_metadata)});
  TRPC_FMT_DEBUG("stream, reset stream end, stream id: {}, status: {}", GetId(), status.ToString());
}

void TrpcStream::SendReset(const Status& status) {
  SetState(State::kClosed);

  TrpcStreamCloseFrameProtocol close_frame{};
  close_frame.fixed_header.stream_id = GetId();
  close_frame.stream_close_metadata.set_close_type(TrpcStreamCloseType::TRPC_STREAM_RESET);
  close_frame.stream_close_metadata.set_ret(status.GetFrameworkRetCode());
  close_frame.stream_close_metadata.set_msg(status.ErrorMessage());

  NoncontiguousBuffer buffer;
  if (TRPC_LIKELY(EncodeMessage(&close_frame, &buffer))) {
    Send(std::move(buffer));
  }

  if (GetStreamVar()) {
    GetStreamVar()->AddRpcCallFailureCount(1);
  }

  TRPC_LOG_ERROR("stream error occurs, send reset, stream id:" << GetId());
}

RetCode TrpcStream::SendData(StreamSendMessage&& msg) {
  TrpcStreamDataFrameProtocol data_frame{};
  data_frame.fixed_header.stream_id = GetId();
  data_frame.body = std::move(std::any_cast<NoncontiguousBuffer&&>(std::move(msg.data_provider)));
  data_frame.fixed_header.data_frame_size = data_frame.ByteSizeLong();
  auto payload_size = data_frame.body.ByteSize();

  NoncontiguousBuffer buffer;
  if (TRPC_UNLIKELY(!EncodeMessage(&data_frame, &buffer))) {
    return RetCode::kError;
  }

  if (TRPC_UNLIKELY(!Send(std::move(buffer)))) {
    return RetCode::kError;
  }

  if (GetStreamVar()) {
    GetStreamVar()->AddSendMessageBytes(payload_size);
    GetStreamVar()->AddSendMessageCount(1);
  }

  return RetCode::kSuccess;
}

RetCode TrpcStream::SendFeedback(StreamSendMessage&& msg) {
  TrpcStreamFeedbackFrameProtocol feedback_frame{};
  feedback_frame.fixed_header.stream_id = GetId();
  feedback_frame.stream_feedback_metadata = std::any_cast<TrpcStreamFeedBackMeta&&>(std::move(msg.metadata));

  NoncontiguousBuffer buffer;
  if (TRPC_UNLIKELY(!EncodeMessage(&feedback_frame, &buffer))) {
    return RetCode::kError;
  }

  if (TRPC_UNLIKELY(!Send(std::move(buffer)))) {
    return RetCode::kError;
  }

  return RetCode::kSuccess;
}

RetCode TrpcStream::SendClose(StreamSendMessage&& msg) {
  TrpcStreamCloseFrameProtocol close_frame{};
  close_frame.fixed_header.stream_id = GetId();
  close_frame.stream_close_metadata = std::any_cast<TrpcStreamCloseMeta&&>(std::move(msg.metadata));
  close_frame.fixed_header.data_frame_size = close_frame.ByteSizeLong();

  NoncontiguousBuffer buffer;
  if (TRPC_UNLIKELY(!EncodeMessage(&close_frame, &buffer))) {
    return RetCode::kError;
  }

  if (TRPC_UNLIKELY(!Send(std::move(buffer)))) {
    return RetCode::kError;
  }

  return RetCode::kSuccess;
}

RetCode TrpcStream::HandleFeedback(StreamRecvMessage&& msg) {
  TrpcStreamFeedbackFrameProtocol feedback_frame{};
  auto* buffer = &std::any_cast<NoncontiguousBuffer&>(msg.message);
  if (TRPC_UNLIKELY(!DecodeMessage(buffer, &feedback_frame))) {
    return RetCode::kError;
  }

  if (GetMutableStreamOptions()->fiber_mode && send_flow_controller_ != nullptr) {
    std::unique_lock lk(flow_control_mutex_);
    if (send_flow_controller_->IncreaseWindow(feedback_frame.stream_feedback_metadata.window_size_increment())) {
      flow_control_cond_.notify_all();
    }
  }

  return RetCode::kSuccess;
}

RetCode TrpcStream::HandleData(StreamRecvMessage&& msg) {
  auto* buffer = &std::any_cast<NoncontiguousBuffer&>(msg.message);

  TrpcStreamDataFrameProtocol data_frame{};
  if (TRPC_UNLIKELY(!DecodeMessage(buffer, &data_frame))) {
    return RetCode::kError;
  }

  auto stream_var = GetStreamVar();
  if (TRPC_LIKELY(stream_var)) {
    stream_var->AddRecvMessageBytes(data_frame.ByteSizeLong());
    stream_var->AddRecvMessageCount(1);
  }

  // Notify that a message has arrived and hand over the data to the application layer for processing.
  OnData(std::move(data_frame.body));

  return RetCode::kSuccess;
}

bool TrpcStream::EncodeMessage(Protocol* protocol, NoncontiguousBuffer* buffer) {
  if (TRPC_LIKELY(protocol->ZeroCopyEncode(*buffer))) {
    return true;
  }

  Status status{GetEncodeErrorCode(), 0, "encode trpc frame failed, stream id:" + std::to_string(GetId())};
  TRPC_LOG_ERROR(status.ErrorMessage());

  // Return a Reset frame from the peer to indicate an exception.
  SendReset(status);
  // Push an error to the RPC interface of the business operation stream.
  OnError(status);

  return false;
}

bool TrpcStream::DecodeMessage(NoncontiguousBuffer* buffer, Protocol* protocol) {
  if (TRPC_LIKELY(protocol->ZeroCopyDecode(*buffer))) {
    return true;
  }

  Status status{GetDecodeErrorCode(), 0, "decode trpc frame failed, stream id:" + std::to_string(GetId())};
  TRPC_LOG_ERROR(status.ErrorMessage());

  // Return a Reset frame from the peer to indicate an exception.
  SendReset(status);
  // Push an error to the RPC interface of the business operation stream.
  OnError(status);

  return false;
}

}  // namespace trpc::stream
