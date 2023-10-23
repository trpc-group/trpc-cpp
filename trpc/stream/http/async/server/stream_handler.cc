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

#include "trpc/stream/http/async/server/stream_handler.h"

#include "trpc/server/server_context.h"
#include "trpc/stream/http/async/server/stream.h"

namespace trpc::stream {

StreamReaderWriterProviderPtr HttpServerAsyncStreamHandler::CreateStream(StreamOptions&& options) {
  if (conn_closed_) {
    TRPC_FMT_ERROR("connection will be closed, reject creation of new stream");
    return nullptr;
  }

  if (stream_) {
    TRPC_FMT_ERROR("stream already exist");
    return nullptr;
  }

  ServerContextPtr context = std::any_cast<ServerContextPtr>(options.context.context);
  options.connection_id = GetMutableStreamOptions()->connection_id;
  stream_ = MakeRefCounted<HttpServerAsyncStream>(std::move(options));
  context->SetStreamReaderWriterProvider(stream_);
  return stream_;
}

}  // namespace trpc::stream
