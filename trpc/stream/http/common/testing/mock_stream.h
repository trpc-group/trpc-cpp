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

#include "trpc/stream/http/common/stream.h"
#include "trpc/stream/http/common/stream_handler.h"

namespace trpc::testing {

class MockHttpCommonStream : public stream::HttpCommonStream {
 public:
  using State = HttpCommonStream::State;
  using Action = HttpCommonStream::Action;

  explicit MockHttpCommonStream(stream::StreamOptions&& options) : HttpCommonStream(std::move(options)) {}

  MOCK_METHOD(void, OnData, (NoncontiguousBuffer*), (override));
  MOCK_METHOD(void, OnEof, (), (override));
  MOCK_METHOD(void, OnHeader, (http::HttpHeader*), (override));
  MOCK_METHOD(void, OnTrailer, (http::HttpHeader*), (override));
  MOCK_METHOD(void, OnError, (Status), (override));

  void PushRecvMessage(stream::HttpStreamFramePtr&& msg) {}

  void SetWriteState(State state) {
    write_state_ = state;
    write_bytes_ = 0;
  }
  State GetWriteState() { return write_state_; }
  void SetReadState(State state) { read_state_ = state; }
  State GetReadState() { return read_state_; }

  stream::RetCode HandleRecvMessage(stream::HttpStreamFramePtr&& msg) override {
    return HttpCommonStream::HandleRecvMessage(std::move(msg));
  }
  stream::RetCode HandleSendMessage(stream::HttpStreamFramePtr&& msg, NoncontiguousBuffer* out) override {
    return HttpCommonStream::HandleSendMessage(std::move(msg), out);
  }
};

class MockHttpCommonStreamHandler : public stream::HttpCommonStreamHandler {
 public:
  explicit MockHttpCommonStreamHandler(stream::StreamOptions&& options) : HttpCommonStreamHandler(std::move(options)) {
    parse_state_ = ParseState::kAfterParseStartLine;
  }

  stream::StreamReaderWriterProviderPtr CreateStream(stream::StreamOptions&& options) override {
    stream_ = MakeRefCounted<MockHttpCommonStream>(std::move(options));
    return stream_;
  }
};

}  // namespace trpc::testing
