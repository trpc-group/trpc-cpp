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

#include "trpc/stream/trpc/trpc_client_stream_handler.h"

#include "trpc/client/client_context.h"
#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/stream/trpc/trpc_client_stream.h"

namespace trpc::stream {

int TrpcClientStreamHandler::GetNetworkErrorCode() { return TrpcRetCode::TRPC_STREAM_CLIENT_NETWORK_ERR; }

StreamReaderWriterProviderPtr TrpcClientStreamHandler::CreateStream(StreamOptions&& options) {
  auto stream_id = stream_id_generator_.GenerateId();
  options.stream_id = stream_id;

  if (CriticalSection<bool>([this]() { return conn_closed_; })) {
    TRPC_LOG_ERROR("connection will be closed, reject creation of new stream: " << stream_id);
    return nullptr;
  }

  auto stream = MakeRefCounted<TrpcClientStream>(std::move(options));
  stream->SetFilterController(&filter_controller_);

  return CriticalSection<RefPtr<TrpcClientStream>>([this, stream_id, stream]() {
    if (conn_closed_) {
      TRPC_LOG_ERROR("connection will be closed, reject creation of new stream: " << stream_id);
      return RefPtr<TrpcClientStream>();
    }
    auto found = streams_.find(stream_id);
    if (found != streams_.end()) {
      TRPC_LOG_ERROR("stream " << stream_id << " already exists.");
      return RefPtr<TrpcClientStream>();
    }

    streams_.emplace(stream_id, stream);
    TRPC_FMT_TRACE("Stream {} has been created", stream_id);
    return stream;
  });
}

int TrpcClientStreamHandler::SendMessage(const std::any& msg, NoncontiguousBuffer&& send_data) {
  IoMessage io_msg;
  io_msg.buffer = std::move(send_data);
  io_msg.msg = msg;

  if (options_.send(std::move(io_msg)) != 0) {
    return GetNetworkErrorCode();
  }

  return 0;
}

}  //  namespace trpc::stream
