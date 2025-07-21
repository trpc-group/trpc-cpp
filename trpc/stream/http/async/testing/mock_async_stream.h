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

#include "trpc/stream/http/async/stream.h"

namespace trpc::testing {

class MockHttpAsyncStream : public stream::HttpAsyncStream {
 public:
  using State = HttpCommonStream::State;

  explicit MockHttpAsyncStream(stream::StreamOptions&& options) : HttpAsyncStream(std::move(options)) {}

  void SetReadState(State state) { read_state_ = state; }

  void SetWriteState(State state) {
    write_state_ = state;
    write_bytes_ = 0;
  }

  MOCK_METHOD(void, Reset, (Status), (override));
};

}  // namespace trpc::testing
