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

#include "trpc/codec/server_codec_factory.h"
#include "trpc/stream/http/common/stream_handler.h"

namespace trpc::stream {

/// @brief The basic class for http server StreamHandler. It further implements the server-specific parsing logic for
///        the request line.
class HttpServerCommonStreamHandler : public HttpCommonStreamHandler {
 public:
  explicit HttpServerCommonStreamHandler(StreamOptions&& options) : HttpCommonStreamHandler(std::move(options)) {}

 protected:
  int ParseMessageImpl(NoncontiguousBuffer* in, std::deque<std::any>* out) override;
};

}  // namespace trpc::stream
