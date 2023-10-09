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

#include "trpc/stream/trpc/trpc_stream_handler.h"

#include "trpc/codec/trpc/trpc_protocol.h"

namespace trpc::stream {

int TrpcStreamHandler::RemoveStreamInner(uint32_t stream_id) {
  auto pos = streams_.find(stream_id);
  if (pos != streams_.end()) {
    streams_.erase(pos);
    return 0;
  }
  return -1;
}

int TrpcStreamHandler::RemoveStream(uint32_t stream_id) {
  // When the connection is closed under FIBER thread model, the stream is not allowed to remove itself.
  if (options_.fiber_mode) {
    std::unique_lock<FiberMutex> _(mutex_);
    // When the connection is closed, the map is no longer updated.
    // At this point, it may have entered Join to wait for the stream to end, and when the stream ends, this function
    // will be called, causing a change in the number of maps.
    if (conn_closed_) {
      return -1;
    }
    return RemoveStreamInner(stream_id);
  }
  // This logic is not needed under DEFAULT thread model.
  return RemoveStreamInner(stream_id);
}

bool TrpcStreamHandler::IsNewStream(uint32_t stream_id, uint32_t frame_type) {
  // A new stream meets the following conditions:
  // - it receives an INIT type frame.
  // - the stream has not been created before.
  // !!! NOTE: It is not yet guaranteed here whether the stream identifier has been used before.
  if (frame_type != static_cast<uint32_t>(TrpcStreamFrameType::TRPC_STREAM_FRAME_INIT)) {
    return false;
  }

  return CriticalSection<bool>([this, stream_id]() { return streams_.find(stream_id) == streams_.end(); });
}

void TrpcStreamHandler::PushMessage(std::any&& message, ProtocolMessageMetadata&& metadata) {
  auto stream = CriticalSection<TrpcStreamPtr>([this, stream_id = metadata.stream_id]() {
    auto pos = streams_.find(stream_id);
    if (pos != streams_.end()) {
      return pos->second;
    } else {
      return TrpcStreamPtr();
    }
  });

  if (TRPC_LIKELY(stream))
    stream->PushRecvMessage(StreamRecvMessage{.message = std::move(message), .metadata = std::move(metadata)});
  else
    TRPC_LOG_ERROR("stream not found, stream id: " << metadata.stream_id);
}

void TrpcStreamHandler::Stop() {
  std::stringstream error_msg;
  error_msg << "error: connection has been closed, conn_id:" << options_.connection_id;
  Status status{GetNetworkErrorCode(), 0, error_msg.str()};
  TRPC_FMT_DEBUG("is_server: {}, err: {}", GetMutableStreamOptions()->server_mode, status.ErrorMessage());

  CriticalSection<void>([this, status]() {
    conn_closed_ = true;
    if (options_.fiber_mode) {
      for (auto& [id, stream] : streams_) {
        TRPC_FMT_DEBUG("push reset to stream, is_server: {}, stream_id: {}", GetMutableStreamOptions()->server_mode,
                       id);
        stream->PushRecvMessage(CreateResetMessage(id, status));
        TRPC_FMT_DEBUG("push reset to stream done, is_server: {}, stream_id: {}",
                       GetMutableStreamOptions()->server_mode, id);
      }
    } else {
      while (!streams_.empty()) {
        auto id = streams_.begin()->first;
        auto stream = streams_.begin()->second;
        TRPC_FMT_DEBUG("push reset to stream, is_server: {}, stream_id: {}", GetMutableStreamOptions()->server_mode,
                       id);
        stream->PushRecvMessage(CreateResetMessage(id, status));  // stream removed inside
        TRPC_FMT_DEBUG("push reset to stream done, is_server: {}, stream_id: {}",
                       GetMutableStreamOptions()->server_mode, id);
      }
    }
  });
}

void TrpcStreamHandler::Join() {
  // Locking is not necessary here for the following two reasons:
  // 1. Whether the remote end actively closes the connection or the local end actively closes the connection, no new
  //    streams will be established, and the number of streams on the map will not increase.
  // 2. If the local end actively closes the connection, the mutex is still needed to push stream frames to existing
  // streams.
  for (auto it = streams_.begin(); it != streams_.end(); ++it) {
    it->second->Join();
  }
  TRPC_FMT_TRACE("connection closed, join streams finish");
  streams_.clear();
}

StreamRecvMessage TrpcStreamHandler::CreateResetMessage(uint32_t stream_id, const Status& status) {
  TrpcStreamCloseFrameProtocol close_frame{};
  close_frame.fixed_header.stream_id = stream_id;
  close_frame.stream_close_metadata.set_close_type(TrpcStreamCloseType::TRPC_STREAM_RESET);
  close_frame.stream_close_metadata.set_ret(status.GetFrameworkRetCode());
  close_frame.stream_close_metadata.set_msg(status.ErrorMessage());

  NoncontiguousBuffer buffer;
  if (TRPC_UNLIKELY(!close_frame.ZeroCopyEncode(buffer))) {
    TRPC_LOG_ERROR("encode trpc frame failed, stream id: " << stream_id);
    assert(false);
  }

  ProtocolMessageMetadata metadata{
      .data_frame_type = static_cast<uint8_t>(TrpcDataFrameType::TRPC_STREAM_FRAME),
      .enable_stream = true,
      .stream_frame_type = static_cast<uint8_t>(TrpcStreamFrameType::TRPC_STREAM_FRAME_CLOSE),
      .stream_id = stream_id};

  return StreamRecvMessage{.message = std::move(buffer), .metadata = std::move(metadata)};
}

}  //  namespace trpc::stream
