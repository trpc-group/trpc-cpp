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

#include "trpc/stream/grpc/grpc_server_stream_handler.h"

#include "trpc/codec/grpc/grpc_protocol.h"
#include "trpc/codec/grpc/grpc_stream_frame.h"
#include "trpc/codec/grpc/http2/server_session.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/server/method.h"
#include "trpc/server/server_context.h"
#include "trpc/stream/grpc/grpc_server_stream.h"

namespace trpc::stream {

GrpcServerStreamHandler::GrpcServerStreamHandler(StreamOptions&& options) : options_(std::move(options)) {
  http2::SessionOptions session_options;
  session_options.send_io_msg = [this](NoncontiguousBuffer&& send_data) {
    if (TRPC_UNLIKELY(!options_.send)) {
      TRPC_LOG_ERROR("no available send function to send io message");
      return -1;
    }
    IoMessage io_msg;
    io_msg.buffer = std::move(send_data);
    return options_.send(std::move(io_msg));
  };
  session_ = std::make_unique<http2::ServerSession>(std::move(session_options));
}

bool GrpcServerStreamHandler::Init() {
  // Send the server HTTP/2 Preface message when initializing.
  NoncontiguousBuffer buffer;
  auto signal_ok = session_->SignalWrite(&buffer);
  if (signal_ok < 0) {
    TRPC_LOG_ERROR("signal write response failed, error: " << signal_ok);
    return false;
  }

  IoMessage io_msg;
  io_msg.buffer = std::move(buffer);

  if (options_.send(std::move(io_msg)) != 0) {
    TRPC_LOG_ERROR("send server http2 preface failed");
    return false;
  }

  // Initialize the callback for the session when sending responses, and use it to distinguish between stream frames
  // and non-stream frames.
  auto& streaming_checker = options_.check_stream_rpc_function;

  // Set the callback for receiving stream headers in HTTP/2.
  session_->SetOnHeaderRecvCallback([this, &streaming_checker](http2::RequestPtr& request) {
    if (streaming_checker(request->GetPath())) {
      // The streaming request checks out the HEADER frame of gRPC.
      GrpcRequestPacket packet;
      packet.frame = MakeRefCounted<GrpcStreamInitFrame>(request->GetStreamId(), request);
      out_.emplace_back(std::move(packet));
    }
  });

  // Set the callback for receiving all HTTP/2 Data frames from one I/O.
  session_->SetOnDataRecvCallback([this, &streaming_checker](http2::RequestPtr& request) {
    if (streaming_checker(request->GetPath())) {
      // The streaming request checks out the DATA frame of gRPC.
      NoncontiguousBuffer* in_buff = request->GetMutableNonContiguousBufferContent();
      do {
        NoncontiguousBuffer out_buff;
        if (!GrpcMessageContent::Check(in_buff, &out_buff)) {
          break;
        }
        auto data_frame = MakeRefCounted<GrpcStreamDataFrame>(request->GetStreamId(), std::move(out_buff));
        GrpcRequestPacket packet;
        packet.frame = data_frame;
        out_.emplace_back(std::move(packet));
      } while (true);
    }
  });

  // Set the callback for receiving EOF.
  session_->SetOnEofRecvCallback([this, &streaming_checker](http2::RequestPtr& request) {
    GrpcRequestPacket packet;
    if (!streaming_checker(request->GetPath())) {
      // Check out the complete request structure for unary requests.
      TRPC_LOG_TRACE("request is ready, stream id: " << request->GetStreamId());
      packet.req = request;
      out_.emplace_back(std::move(packet));
    } else {
      // For server streaming RPC, EOF does not submit a stream close.
      packet.frame = MakeRefCounted<GrpcStreamCloseFrame>(request->GetStreamId());
      out_.emplace_back(std::move(packet));
    }
  });

  // Set the callback for receiving RST.
  session_->SetOnRstRecvCallback([this, &streaming_checker](http2::RequestPtr& request, uint32_t error_code) {
    if (streaming_checker(request->GetPath())) {
      GrpcRequestPacket packet;
      packet.frame = MakeRefCounted<GrpcStreamResetFrame>(request->GetStreamId(), error_code);
    }
  });

  return true;
}

int GrpcServerStreamHandler::ParseMessage(NoncontiguousBuffer* in, std::deque<std::any>* out) {
  if (TRPC_UNLIKELY(session_->SignalRead(in) != 0)) {
    TRPC_LOG_ERROR("parse message failed(server)");
    return -1;
  }
  out->swap(out_);
  return 0;
}

int GrpcServerStreamHandler::EncodeHttp2Response(const http2::ResponsePtr& response, NoncontiguousBuffer* buffer) {
  int submit_ok = session_->SubmitResponse(response);
  if (submit_ok != 0) {
    TRPC_LOG_ERROR("submit response failed, error: " << submit_ok);
    return submit_ok;
  }

  int signal_ok = session_->SignalWrite(buffer);
  if (signal_ok < 0) {
    TRPC_LOG_ERROR("signal write response failed, error: " << signal_ok);
    return signal_ok;
  }

  return 0;
}

namespace {
http2::ResponsePtr GetHttp2Response(const std::any& msg) {
  http2::ResponsePtr http2_response{nullptr};
  try {
    const auto& context = std::any_cast<const ServerContextPtr&>(msg);
    auto grpc_unary_response = static_cast<GrpcUnaryResponseProtocol*>(context->GetResponseMsg().get());
    http2_response = grpc_unary_response->GetHttp2Response();
  } catch (std::bad_any_cast& e) {
    TRPC_LOG_ERROR("exception: " << e.what() << ", " << msg.type().name());
  }
  return http2_response;
}
}  // namespace

int DefaultGrpcServerStreamHandler::EncodeTransportMessage(IoMessage* msg) {
  http2::ResponsePtr http2_response = GetHttp2Response(msg->msg);
  if (TRPC_UNLIKELY(!http2_response)) {
    return -1;
  }
  if (TRPC_UNLIKELY(EncodeHttp2Response(http2_response, &(msg->buffer)) != 0)) {
    return -1;
  }
  return 0;
}

bool FiberGrpcServerStreamHandler::Init() {
  GrpcServerStreamHandler::Init();
  // Currently, only connection-level flow control is implemented.
  session_->SetOnWindowUpdateRecvCallback([this]() { session_cv_.notify_all(); });
  return true;
}

int FiberGrpcServerStreamHandler::EncodeTransportMessage(IoMessage* msg) {
  http2::ResponsePtr http2_response = GetHttp2Response(msg->msg);
  if (TRPC_UNLIKELY(!http2_response)) {
    return -1;
  }

  {
    // In fiber mode, there is a race condition here, so locking protection is added.
    std::unique_lock lock(session_mutex_);
    if (TRPC_UNLIKELY(EncodeHttp2Response(http2_response, &(msg->buffer)) != 0)) {
      return -1;
    }
  }

  return 0;
}

int FiberGrpcServerStreamHandler::ParseMessage(NoncontiguousBuffer* in, std::deque<std::any>* out) {
  // In fiber mode, there is a race condition here, so locking protection is added.
  std::unique_lock lock(session_mutex_);
  return GrpcServerStreamHandler::ParseMessage(in, out);
}

int FiberGrpcServerStreamHandler::RemoveStream(uint32_t stream_id) {
  std::unique_lock lk(stream_mutex_);
  // The connection is closed and the map is no longer updated.
  // It may now enter Join and wait for the stream to end. When the stream end process is called, this function will
  // be invoked, causing the number of maps to change.
  if (conn_closed_) {
    return -1;
  }
  auto pos = streams_.find(stream_id);
  if (pos != streams_.end()) {
    streams_.erase(pos);
    return 0;
  }
  return -1;
}

bool FiberGrpcServerStreamHandler::IsNewStream(uint32_t stream_id, uint32_t frame_type) {
  // The new stream satisfies the following conditions: received INIT frame type, and the stream has not been
  // created before.
  if (frame_type != static_cast<uint32_t>(StreamMessageCategory::kStreamInit)) {
    return false;
  }
  std::unique_lock lock(stream_mutex_);
  return streams_.find(stream_id) == streams_.end();
}

void FiberGrpcServerStreamHandler::PushMessage(std::any&& message, ProtocolMessageMetadata&& metadata) {
  auto packet = std::any_cast<GrpcRequestPacket&&>(std::move(message));
  {
    std::unique_lock lock(stream_mutex_);
    auto pos = streams_.find(metadata.stream_id);
    if (pos != streams_.end()) {
      pos->second->PushRecvMessage(
          StreamRecvMessage{.message = std::move(packet.frame), .metadata = std::move(metadata)});
    }
  }
}

void FiberGrpcServerStreamHandler::Stop() {
  std::unique_lock lk(stream_mutex_);
  conn_closed_ = true;
  TRPC_LOG_TRACE("connection closed, stop streams");
  for (auto& [id, stream] : streams_) {
    stream->PushRecvMessage(CreateResetMessage(id, GetNetworkErrorCode()));
  }
}

void FiberGrpcServerStreamHandler::Join() {
  // No need to lock here, because of the following two reasons:
  // 1. Regardless of whether the remote end actively closes the connection or the local end actively closes the
  // connection, no new streams will be created, and the number of streams in the map will not increase.
  // 2. If the local end actively closes the connection, the mutex is still needed to push stream frames to
  // existing streams.
  for (auto it = streams_.begin(); it != streams_.end(); ++it) {
    it->second->Join();
  }
  TRPC_LOG_TRACE("connection closed, join streams finish");
  streams_.clear();
}

StreamReaderWriterProviderPtr FiberGrpcServerStreamHandler::CreateStream(StreamOptions&& options) {
  // If it is a new stream, create a new stream object and start the stream message processing coroutine.
  {
    std::unique_lock lock(stream_mutex_);
    auto stream_id = options.stream_id;
    // The connection is closed, and new stream creation is rejected. PushMessage will only push stream frames to
    // existing streams.
    if (conn_closed_) {
      TRPC_LOG_ERROR("connection will be closed, reject creation of new stream: " << stream_id);
      return nullptr;
    }
    auto found = streams_.find(stream_id);
    // The stream already exists, and new stream creation is rejected.
    if (found != streams_.end()) {
      TRPC_LOG_ERROR("stream " << stream_id << " already exists.");
      return nullptr;
    }
    ServerContextPtr context = std::any_cast<ServerContextPtr>(options.context.context);
    auto stream = MakeRefCounted<GrpcServerStream>(std::move(options), session_.get(), &session_mutex_, &session_cv_);
    // Set the stream for the context so that it can be obtained in stream_rpc_method_handler.
    context->SetStreamReaderWriterProvider(stream);
    streams_.emplace(stream_id, stream);
    return stream;
  }
  return nullptr;
}

int FiberGrpcServerStreamHandler::SendMessage(const std::any& msg, NoncontiguousBuffer&& send_data) {
  IoMessage io_msg;
  io_msg.buffer = std::move(send_data);
  io_msg.msg = msg;
  if (options_.send(std::move(io_msg)) != 0) {
    return GetNetworkErrorCode();
  }
  return 0;
}

int FiberGrpcServerStreamHandler::GetNetworkErrorCode() { return static_cast<int>(GrpcStatus::kGrpcInternal); }

StreamRecvMessage FiberGrpcServerStreamHandler::CreateResetMessage(uint32_t stream_id, uint32_t error_code) {
  GrpcStreamFramePtr rst_msg = MakeRefCounted<GrpcStreamResetFrame>(stream_id, error_code);

  ProtocolMessageMetadata metadata{.data_frame_type = static_cast<uint8_t>(TrpcDataFrameType::TRPC_STREAM_FRAME),
                                   .enable_stream = true,
                                   .stream_frame_type = static_cast<uint8_t>(StreamMessageCategory::kStreamReset),
                                   .stream_id = stream_id};

  return StreamRecvMessage{.message = std::move(rst_msg), .metadata = std::move(metadata)};
}

}  // namespace trpc::stream
