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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/server_codec.h"
#include "trpc/codec/testing/protocol_testing.h"

namespace trpc::testing {

class TestServerCodec : public ServerCodec {
 public:
  std::string Name() const override { return "TestServerCodec"; }
  ProtocolPtr CreateRequestObject() override { return std::make_shared<TestProtocol>(); }
  ProtocolPtr CreateResponseObject() override { return std::make_shared<TestProtocol>(); }
};

class MockServerCodec : public ServerCodec {
 public:
  std::string Name() const override { return "MockServerCodec"; }
  MOCK_METHOD(int, ZeroCopyCheck, (const ConnectionPtr&, NoncontiguousBuffer&, std::deque<std::any>&), (override));
  MOCK_METHOD(bool, ZeroCopyDecode, (const ServerContextPtr&, std::any&&, ProtocolPtr&), (override));
  MOCK_METHOD(bool, ZeroCopyEncode, (const ServerContextPtr&, ProtocolPtr&, NoncontiguousBuffer&), (override));
  MOCK_METHOD(ProtocolPtr, CreateRequestObject, (), (override));
  MOCK_METHOD(ProtocolPtr, CreateResponseObject, (), (override));
  MOCK_METHOD(bool, Pick, (const std::any&, std::any&), (const override));
  MOCK_METHOD(bool, HasStreamingRpc, (), (const, override));
  MOCK_METHOD(bool, IsStreamingProtocol, (), (const, override));
  MOCK_METHOD(int, GetProtocolRetCode, (codec::ServerRetCode), (const, override));
};

}  // namespace trpc::testing
