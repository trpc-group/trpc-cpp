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

#include "trpc/stream/trpc/trpc_server_stream_handler.h"

#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/server/server_context.h"
#include "trpc/stream/trpc/trpc_server_stream.h"
#include "trpc/util/log/logging.h"

namespace trpc::stream {

int TrpcServerStreamHandler::GetNetworkErrorCode() { return TrpcRetCode::TRPC_STREAM_SERVER_NETWORK_ERR; }

StreamReaderWriterProviderPtr TrpcServerStreamHandler::CreateStream(StreamOptions&& options) {
  // If it is a new stream, create a stream object and start the stream message processing coroutine.
  auto stream_id = options.stream_id;
  // The connection is closed, new stream establishment is rejected, and PushMessage only pushes stream frames to
  // existing streams.
  if (CriticalSection<bool>([this]() { return conn_closed_; })) {
    TRPC_LOG_ERROR("connection will be closed, reject creation of new stream: " << stream_id);
    return nullptr;
  }

  // Since IsNewStream has previously determined that the stream does not exist, it is likely that it still does not
  // exist, so create the stream first.
  options.connection_id = options_.connection_id;
  ServerContextPtr context = std::any_cast<ServerContextPtr>(options.context.context);
  auto stream = MakeRefCounted<TrpcServerStream>(std::move(options));
  context->SetStreamReaderWriterProvider(stream);

  return CriticalSection<RefPtr<TrpcServerStream>>([this, stream_id, stream]() {
    if (conn_closed_) {
      TRPC_LOG_ERROR("connection will be closed, reject creation of new stream: " << stream_id);
      return RefPtr<TrpcServerStream>();
    }
    auto found = streams_.find(stream_id);
    if (found != streams_.end()) {
      TRPC_LOG_ERROR("stream " << stream_id << " already exists.");
      return RefPtr<TrpcServerStream>();
    }

    streams_.emplace(stream_id, stream);
    TRPC_FMT_TRACE("Stream {} has been created", stream_id);
    return stream;
  });
}

int TrpcServerStreamHandler::SendMessage(const std::any& msg, NoncontiguousBuffer&& send_data) {
  IoMessage io_msg;
  io_msg.buffer = std::move(send_data);
  io_msg.msg = msg;

  if (options_.send(std::move(io_msg)) != 0) {
    return GetNetworkErrorCode();
  }

  return 0;
}

}  //  namespace trpc::stream
