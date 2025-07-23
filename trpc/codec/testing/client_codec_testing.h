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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/client_codec.h"

namespace trpc::testing {

class TestClientCodec : public ClientCodec {
 public:
  TestClientCodec() = default;
  ~TestClientCodec() override = default;

  std::string Name() const override { return "TestClientCodec"; }
  ProtocolPtr CreateRequestPtr() override { return nullptr; }
  ProtocolPtr CreateResponsePtr() override { return nullptr; }
};

class MockClientCodec : public ClientCodec {
 public:
  std::string Name() const override { return "MockClientCodec"; }
  MOCK_METHOD(int, ZeroCopyCheck, (const ConnectionPtr&, NoncontiguousBuffer&, std::deque<std::any>&), (override));
  MOCK_METHOD(bool, ZeroCopyDecode, (const ClientContextPtr&, std::any&&, ProtocolPtr&), (override));
  MOCK_METHOD(bool, ZeroCopyEncode, (const ClientContextPtr&, const ProtocolPtr&, NoncontiguousBuffer&), (override));
  MOCK_METHOD(bool, FillRequest, (const ClientContextPtr&, const ProtocolPtr&, void*), (override));
  MOCK_METHOD(bool, FillResponse, (const ClientContextPtr&, const ProtocolPtr&, void*), (override));
  MOCK_METHOD(ProtocolPtr, CreateRequestPtr, (), (override));
  MOCK_METHOD(ProtocolPtr, CreateResponsePtr, (), (override));
  MOCK_METHOD(uint32_t, GetSequenceId, (const ProtocolPtr&), (const, override));
  MOCK_METHOD(bool, Pick, (const std::any&, std::any&), (const override));
  MOCK_METHOD(bool, IsComplex, (), (const, override));
  MOCK_METHOD(bool, HasStreamingRpc, (), (const, override));
  MOCK_METHOD(bool, IsStreamingProtocol, (), (const, override));
  MOCK_METHOD(int, GetProtocolRetCode, (codec::ClientRetCode), (const, override));
};
}  // namespace trpc::testing
