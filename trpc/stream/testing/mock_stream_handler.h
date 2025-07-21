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

#include <string>

#include "gmock/gmock.h"

#include "trpc/stream/stream_handler.h"

namespace trpc::testing {

class MockStreamHandler : public stream::StreamHandler {
 public:
  MOCK_METHOD(bool, Init, (), (override));
  MOCK_METHOD(void, Stop, (), (override));
  MOCK_METHOD(void, Join, (), (override));

  stream::StreamReaderWriterProviderPtr CreateStream(stream::StreamOptions&& options) override {
    return CreateStream_rvr(std::move(options));
  }
  MOCK_METHOD(stream::StreamReaderWriterProviderPtr, CreateStream_rvr, (stream::StreamOptions &&));

  MOCK_METHOD(int, RemoveStream, (uint32_t), (override));
  MOCK_METHOD(bool, IsNewStream, (uint32_t, uint32_t), (override));

  void PushMessage(std::any&& message, stream::ProtocolMessageMetadata&& metadata) override {
    PushMessage_rvr(std::move(message), std::move(metadata));
  }
  MOCK_METHOD(void, PushMessage_rvr, (std::any&&, stream::ProtocolMessageMetadata&&));

  int SendMessage(const std::any& data, NoncontiguousBuffer&& buff) override {
    return SendMessage_rvr(data, std::move(buff));
  }
  MOCK_METHOD(int, SendMessage_rvr, (const std::any&, NoncontiguousBuffer&&));

  virtual void OnConnectionClose(int data, std::string&& buff) { return OnConnectionClose_rvr(data, std::move(buff)); }
  MOCK_METHOD(void, OnConnectionClose_rvr, (int, std::string&&));
  MOCK_METHOD(int, ParseMessage, (NoncontiguousBuffer*, std::deque<std::any>*), (override));
  MOCK_METHOD(int, EncodeTransportMessage, (IoMessage*), (override));
  MOCK_METHOD(stream::StreamOptions*, GetMutableStreamOptions, (), (override));
};

using MockStreamHandlerPtr = RefPtr<MockStreamHandler>;

inline MockStreamHandlerPtr CreateMockStreamHandler() { return MakeRefCounted<MockStreamHandler>(); }

}  // namespace trpc::testing
