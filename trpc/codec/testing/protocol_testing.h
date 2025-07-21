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

#include "trpc/codec/protocol.h"

namespace trpc::testing {

class TestProtocol : public Protocol {
  // @brief Decodes or encodes a protocol message (zero copy).
  bool ZeroCopyDecode(NoncontiguousBuffer& buff) override { return true; }
  bool ZeroCopyEncode(NoncontiguousBuffer& buff) override { return true; }
};

using TestProtocolPtr = std::shared_ptr<TestProtocol>;

class MockProtocol : public trpc::Protocol {
 public:
  MOCK_METHOD(bool, ZeroCopyDecode, (NoncontiguousBuffer&), (override));
  MOCK_METHOD(bool, ZeroCopyEncode, (NoncontiguousBuffer&), (override));

  MOCK_METHOD(bool, WaitForFullRequest, (), (override));

  MOCK_METHOD(void, SetNonContiguousProtocolBody, (NoncontiguousBuffer&&), (override));
  MOCK_METHOD(NoncontiguousBuffer, GetNonContiguousProtocolBody, (), (override));

  MOCK_METHOD(void, SetProtocolAttachment, (NoncontiguousBuffer&&), (override));
  MOCK_METHOD(NoncontiguousBuffer, GetProtocolAttachment, (), (override));

  MOCK_METHOD(bool, SetRequestId, (uint32_t req_id), (override));
  MOCK_METHOD(bool, GetRequestId, (uint32_t&), (const, override));

  MOCK_METHOD(void, SetTimeout, (uint32_t), (override));
  MOCK_METHOD(uint32_t, GetTimeout, (), (const, override));

  MOCK_METHOD(void, SetCallerName, (const std::string&));
  MOCK_METHOD(const std::string&, GetCallerName, (), (const, override));

  MOCK_METHOD(void, SetCalleeName, (const std::string&));
  MOCK_METHOD(const std::string&, GetCalleeName, (), (const, override));

  MOCK_METHOD(void, SetFuncName, (const std::string&));
  MOCK_METHOD(const std::string&, GetFuncName, (), (const, override));
  // Something other ...
};

}  // namespace trpc::testing
