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

#include "trpc/stream/http/http_client_stream.h"

namespace trpc::stream {

/// @private For internal use purpose only.
class HttpClientStreamResponse {
 public:
  inline auto& GetStream() { return stream_; }

  inline std::any& GetData() { return data_; }

  inline void SetData(std::any&& data) { data_ = std::move(data); }

 private:
  HttpClientStreamPtr stream_;
  std::any data_;
};

using HttpClientStreamResponsePtr = std::shared_ptr<HttpClientStreamResponse>;

}  // namespace trpc::stream
