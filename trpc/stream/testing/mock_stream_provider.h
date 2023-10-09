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

#include <string>

#include "gmock/gmock.h"

#include "trpc/stream/stream_provider.h"

namespace trpc::testing {

class MockStreamReaderWriterProvider : public stream::StreamReaderWriterProvider {
 public:
  MockStreamReaderWriterProvider() {}
  ~MockStreamReaderWriterProvider() override = default;

  MOCK_METHOD(Status, Read, (NoncontiguousBuffer*, int), (override));
  MOCK_METHOD(Status, Write, (NoncontiguousBuffer&&), (override));
  MOCK_METHOD(Status, WriteDone, (), (override));
  MOCK_METHOD(Status, Start, (), (override));
  MOCK_METHOD(Status, Finish, (), (override));
  MOCK_METHOD(void, Close, (Status), (override));
  MOCK_METHOD(void, Reset, (Status), (override));
  MOCK_METHOD(Status, GetStatus, (), (const, override));
  MOCK_METHOD(stream::StreamOptions*, GetMutableStreamOptions, (), (override));

  MOCK_METHOD(Future<std::optional<NoncontiguousBuffer>>, AsyncRead, (int), (override));
  MOCK_METHOD(Future<>, AsyncWrite, (NoncontiguousBuffer&&), (override));
  MOCK_METHOD(Future<>, AsyncStart, (), (override));
  MOCK_METHOD(Future<>, AsyncFinish, (), (override));
};

using MockStreamReaderWriterProviderPtr = RefPtr<MockStreamReaderWriterProvider>;

inline MockStreamReaderWriterProviderPtr CreateMockStreamReaderWriterProvider() {
  return MakeRefCounted<MockStreamReaderWriterProvider>();
}

}  // namespace trpc::testing
