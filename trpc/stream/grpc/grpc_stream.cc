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

#include "trpc/stream/grpc/grpc_stream.h"

#include <sstream>

#include "trpc/codec/grpc/grpc_protocol.h"
#include "trpc/codec/grpc/grpc_stream_frame.h"

namespace trpc::stream {

int GrpcStream::GetEncodeErrorCode() { return GrpcStatus::kGrpcInternal; }

int GrpcStream::GetDecodeErrorCode() { return GrpcStatus::kGrpcInternal; }

int GrpcStream::GetReadTimeoutErrorCode() { return GrpcStatus::kGrpcDeadlineExceeded; }

void GrpcStream::Close(Status status) {
  TRPC_LOG_DEBUG("stream, close stream begin, stream id: " << GetId() << ", status: " << status.ToString());
  PushSendMessage(StreamSendMessage{
      .category = StreamMessageCategory::kStreamClose, .data_provider = nullptr, .metadata = std::move(status)});
  TRPC_LOG_DEBUG("stream, close stream end, stream id: " << GetId() << ", status: " << status.ToString());
}

void GrpcStream::Reset(Status status) {
  TRPC_LOG_DEBUG("stream, reset stream begin, stream id: " << GetId() << ", status: " << status.ToString());
  int error_code = status.GetFrameworkRetCode();
  TRPC_LOG_ERROR(status.ErrorMessage());
  PushSendMessage(StreamSendMessage{
      .category = StreamMessageCategory::kStreamReset, .data_provider = nullptr, .metadata = error_code});
  TRPC_LOG_DEBUG("stream, reset stream end, stream id: " << GetId() << ", status: " << status.ToString());
}

void GrpcStream::SendReset(uint32_t error_code) {
  SetState(State::kClosed);

  NoncontiguousBuffer buffer;
  if (TRPC_LIKELY(EncodeMessage(EncodeMessageType::kReset, std::move(error_code), &buffer))) {
    Send(std::move(buffer));
  }

  if (GetStreamVar()) {
    GetStreamVar()->AddRpcCallFailureCount(1);
  }

  TRPC_LOG_ERROR("stream error occurs, send reset, stream id:" << GetId());
}

FiberStreamJobScheduler::RetCode GrpcStream::HandleReset(StreamRecvMessage&& msg) {
  // Terminate the stream when there is an error on the client without checking the state.
  auto frame = static_cast<GrpcStreamResetFrame*>(std::any_cast<GrpcStreamFramePtr&>(msg.message).get());

  std::stringstream ss;
  ss << "Client reset stream, error_code: " << frame->GetErrorCode() << ", stream_id: " << GetId();
  Status status{GetEncodeErrorCode(), 0, ss.str()};
  TRPC_LOG_ERROR(status.ErrorMessage());

  OnError(status);

  // The stream has been terminated abnormally, and the server no longer needs to handle the stream.
  // Returning kError means that there is an error in the stream processing, which occurred in the client stream
  // processing here.
  return RetCode::kError;
}

FiberStreamJobScheduler::RetCode GrpcStream::SendReset(StreamSendMessage&& msg) {
  int error_code = std::any_cast<int&&>(std::move(msg.metadata));
  SendReset(error_code);
  if (GetStreamVar()) {
    GetStreamVar()->AddRpcCallFailureCount(1);
  }
  // The stream has been terminated abnormally, and the server no longer needs to handle the stream.
  // Returning kError means that there is an error in the stream processing, which occurred in the client stream
  // processing here.
  return RetCode::kError;
}

bool GrpcStream::EncodeMessage(EncodeMessageType type, std::any&& msg, NoncontiguousBuffer* buffer) {
  std::unique_lock lk(*mutex_, std::defer_lock);
  std::string err_msg;
  bool has_error = false;
  switch (type) {
    case EncodeMessageType::kHeader: {
      auto&& header = std::any_cast<http2::HeaderPairs&&>(std::move(msg));
      lk.lock();
      int submit_ok = session_->SubmitHeader(GetId(), header);
      if (submit_ok != 0) {
        err_msg = "Error SubmitHeader";
        has_error = true;
      }
      break;
    }
    case EncodeMessageType::kData: {
      auto data = std::any_cast<NoncontiguousBuffer&&>(std::move(msg));
      lk.lock();
      int submit_ok = session_->SubmitData(GetId(), std::move(data));
      if (submit_ok != 0) {
        err_msg = "Error SubmitData";
        has_error = true;
      }
      break;
    }
    case EncodeMessageType::kTrailer: {
      auto trailer = std::any_cast<http2::TrailerPairs&&>(std::move(msg));
      lk.lock();
      int submit_ok = session_->SubmitTrailer(GetId(), trailer);
      if (submit_ok != 0) {
        err_msg = "Error SubmitTrailer";
        has_error = true;
      }
      break;
    }
    case EncodeMessageType::kReset: {
      auto error_code = std::any_cast<uint32_t>(std::move(msg));
      lk.lock();
      int submit_ok = session_->SubmitReset(GetId(), error_code);
      if (submit_ok != 0) {
        err_msg = "Error SubmitReset";
        has_error = true;
      }
      break;
    }
    default:
      has_error = true;
      err_msg = "Error encode message type: " + std::to_string(static_cast<int>(type));
  }

  if (!has_error) {
    int signal_ok = session_->SignalWrite(buffer);
    if (signal_ok < 0) {
      err_msg = "signal write response failed, error: " + std::to_string(signal_ok);
      has_error = true;
    }
  }

  if (has_error) {
    err_msg += ", conn_id: " + std::to_string(GetMutableStreamOptions()->connection_id) +
               ", stream_id: " + std::to_string(GetId());
    Status status{GetEncodeErrorCode(), 0, std::move(err_msg)};
    TRPC_LOG_ERROR(status.ErrorMessage());

    OnError(status);
    // Special handling is required for resetting failed processes to prevent recursive calls.
    if (type != EncodeMessageType::kReset) {
      // Resetting requires encoding, so unlocking occurs to prevent deadlocks.
      lk.unlock();
      SendReset(GetEncodeErrorCode());
    }
  }

  return !has_error;
}

}  // namespace trpc::stream
