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

#include "trpc/stream/http/async/client/stream_handler.h"

#include "trpc/stream/http/async/client/stream.h"

namespace trpc::stream {

StreamReaderWriterProviderPtr HttpClientAsyncStreamHandler::CreateStream(StreamOptions&& options) {
  if (conn_closed_) {
    TRPC_FMT_ERROR("connection will be closed, reject creation of new stream");
    return nullptr;
  }
  if (stream_ != nullptr) {
    TRPC_FMT_ERROR("stream has not been closed, reject creation of new stream");
    return nullptr;
  }
  stream_ = MakeRefCounted<HttpClientAsyncStream>(std::move(options));
  return stream_;
}

}  // namespace trpc::stream
