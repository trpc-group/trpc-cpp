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

#include "gmock/gmock.h"

#include "trpc/stream/http/async/server/stream.h"

namespace trpc::testing {

class MockHttpServerAsyncStream : public stream::HttpServerAsyncStream {
 public:
  using State = HttpCommonStream::State;

  explicit MockHttpServerAsyncStream(stream::StreamOptions&& options) : HttpServerAsyncStream(std::move(options)) {}

  State GetReadState() { return read_state_; }

  State GetWriteState() { return write_state_; }

  int GetReadTimeoutErrorCode() override { return HttpServerAsyncStream::GetReadTimeoutErrorCode(); }

  int GetWriteTimeoutErrorCode() override { return HttpServerAsyncStream::GetWriteTimeoutErrorCode(); }

  int GetNetWorkErrorCode() override { return HttpServerAsyncStream::GetNetWorkErrorCode(); }
};

}  // namespace trpc::testing
