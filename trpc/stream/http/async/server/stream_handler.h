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

#pragma once

#include "trpc/filter/server_filter_controller.h"
#include "trpc/stream/http/common/server/stream_handler.h"

namespace trpc::stream {

/// @brief The server asynchronous StreamHandler of http
class HttpServerAsyncStreamHandler : public HttpServerCommonStreamHandler {
 public:
  explicit HttpServerAsyncStreamHandler(StreamOptions&& options) : HttpServerCommonStreamHandler(std::move(options)) {}

  StreamReaderWriterProviderPtr CreateStream(StreamOptions&& options) override;
};

}  // namespace trpc::stream
