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

#pragma once

#include "trpc/stream/http/common/client/stream_handler.h"

namespace trpc::stream {

/// @brief The asynchronous client stream handler of http.
class HttpClientAsyncStreamHandler : public HttpClientCommonStreamHandler {
 public:
  explicit HttpClientAsyncStreamHandler(StreamOptions&& options) : HttpClientCommonStreamHandler(std::move(options)) {}

  StreamReaderWriterProviderPtr CreateStream(StreamOptions&& options) override;
};

}  // namespace trpc::stream
